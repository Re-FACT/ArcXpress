import yaml
import xml.etree.ElementTree as ET
import argparse
import sys
import logging
import xml.dom.minidom
import os
import re

logging.basicConfig(level=logging.INFO, format='%(message)s')


# Load the master config YAML (input.yaml) to get the list of actual YAML files to process
def load_config_file(config_file):
    try:
        with open(config_file, 'r') as f:
            config = yaml.safe_load(f)
        yaml_files = config.get("qorso_config_tasks", [])
        if not yaml_files:
            raise ValueError("No YAML files listed under 'qorso_config_tasks' in the input config.")
        return yaml_files
    except Exception as e:
        raise ValueError(f"Failed to load config file '{config_file}': {e}")


# Load actual timing YAML content from listed YAML files
def load_multiple_yaml(file_list):
    combined_data = {
        "report_timing": [],
        "timing_summary": []
    }
    for file in file_list:
        with open(file, 'r') as f:
            data = yaml.safe_load(f)
            combined_data["report_timing"].extend(data.get("report_timing", []))
            combined_data["timing_summary"].extend(data.get("timing_summary", []))
    return combined_data


# Add pin prefix inside [get_ports ...] and [get_pins ...]
def add_pin_prefix(pin_text, prefix):
    if not prefix:
        return pin_text

    def repl(match):
        cmd = match.group(1)
        content = match.group(2)
        if content.startswith(prefix + "/") or content.startswith("/") or content.startswith("*"):
            return f"[{cmd} {content}]"
        return f"[{cmd} {prefix}{content}]"

    return re.sub(r"\[(get_ports|get_pins|get_clocks) ([^\]]+)\]", repl, pin_text)


# Build the XML structure
def build_xml(yaml_data, custom_sdc_dir, design_override=None, scaling_factor_override=None, pin_prefix=None, verbose=False):
    root = ET.Element("qorso_strategies")
    task_map = {task["name"]: task for task in yaml_data["report_timing"]}

    seen_strategies = set()

    for summary in yaml_data["timing_summary"]:
        strategy_name = summary["yaml"]["variable"][0]

        if strategy_name in seen_strategies:
            raise ValueError(f"Duplicate strategy name detected: '{strategy_name}'. Please ensure unique strategy names.")
        seen_strategies.add(strategy_name)

        strategy_elem = ET.SubElement(root, "strategy", name=strategy_name, type="timing")

        summary_data_type = summary["yaml"]["data_type"]
        timing_data = summary["yaml"]["timing_data"]
        scale_factor = scaling_factor_override if scaling_factor_override is not None else summary["yaml"].get("scale_factor", 1)

        summary_elem = ET.SubElement(
            strategy_elem,
            "summary",
            timing_data=timing_data,
            data_type=summary_data_type,
            scale_factor=str(scale_factor)
        )

        var_name = summary["yaml"]["variable"][0]
        var_label = summary["csv"]["variable"][0]
        ET.SubElement(summary_elem, "variable", name=var_name).text = var_label

        for task_name in summary["timing_tasks"]:
            if verbose:
                logging.info(f"  - Processing task: {task_name}")
            task = task_map.get(task_name)
            if not task:
                logging.info(f"  Warning: Task '{task_name}' not found in report_timing.")
                continue

            task_elem = ET.SubElement(strategy_elem, "task", name=task_name)

            designs_elem = ET.SubElement(task_elem, "designs")
            if design_override:
                for dname in design_override.split(','):
                    ET.SubElement(designs_elem, "design").text = dname.strip()
            else:
                for design in task.get("current_design", []):
                    ET.SubElement(designs_elem, "design").text = design

            path_attrs = {
                "max_delay": str(task["path"].get("max_delay", 0)),
                "min_delay": str(task["path"].get("min_delay", 0)),
                "delay_type": task["path"]["delay_type"],
                "clock_group": task["path"].get("clock_group", ""),
                "nworst": str(task["path"].get("nworst", 1)),
                "max_paths": str(task["path"].get("max_paths", 1000)),
            }
            path_elem = ET.SubElement(task_elem, "path", **path_attrs)

            from_pins_elem = ET.SubElement(path_elem, "from_pins")
            for pin in task["path"]["from_pin"]:
                ET.SubElement(from_pins_elem, "pin").text = add_pin_prefix(pin, pin_prefix)

            to_pins_elem = ET.SubElement(path_elem, "to_pins")
            for pin in task["path"]["to_pin"]:
                ET.SubElement(to_pins_elem, "pin").text = add_pin_prefix(pin, pin_prefix)

            sdc_elem = ET.SubElement(task_elem, "sdc")
            for sdc in task.get("sdc", []):
                if "sdc/" in sdc:
                    filename = sdc.split("sdc/", 1)[1]
                    path = custom_sdc_dir + "/" + filename
                else:
                    path = sdc
                ET.SubElement(sdc_elem, "file").text = path

    return root


# Write XML with pretty format
def write_xml(xml_elem, out_file):
    rough_string = ET.tostring(xml_elem, encoding="utf-8")
    reparsed = xml.dom.minidom.parseString(rough_string)
    pretty_xml = reparsed.toprettyxml(indent="  ")
    pretty_xml_clean = "\n".join([line for line in pretty_xml.splitlines() if line.strip()])

    with open(out_file, "w", encoding="utf-8") as f:
        f.write(pretty_xml_clean)


# Entry point
def main():
    parser = argparse.ArgumentParser(description="Convert Qorso timing YAMLs to a single XML output (via config).")
    parser.add_argument('--input', '-i', required=True, help='Main config YAML file listing task YAMLs')
    parser.add_argument('--output', '-o', required=True, help='Output XML file')
    parser.add_argument('--verbose', '-v', action='store_true', help='Enable verbose output')
    parser.add_argument('--design_name', type=str, help='Override design name')
    parser.add_argument('--scaling_factor', type=float, help='Override scaling factor')
    parser.add_argument('--pin_prefix', type=str, help='Prefix to add to pin paths')
    parser.add_argument('--custom_sdc_dir', type=str, help='Custom SDC directory path')
    args = parser.parse_args()

    try:
        # Step 1: Load config.yaml (contains list of YAML file names)
        yaml_file_list = load_config_file(args.input)

        # Step 2: Load all actual YAMLs from that list
        yaml_data = load_multiple_yaml(yaml_file_list)

        if args.verbose:
            logging.info("Loaded YAML files from config:")
            for f in yaml_file_list:
                logging.info(f)

        # Step 3: Build XML
        xml_elem = build_xml(
            yaml_data,
            args.custom_sdc_dir,
            design_override=args.design_name,
            scaling_factor_override=args.scaling_factor,
            pin_prefix=args.pin_prefix,
            verbose=args.verbose
        )
        write_xml(xml_elem, args.output)
        logging.info(f"XML written to: {args.output}")

    except ValueError as ve:
        logging.error(str(ve))
        sys.exit(1)
    except Exception as e:
        logging.error(f"Unexpected error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()

#include "floorplan_qt.h"

#ifdef USE_QT

#include <QApplication>
#include <QBrush>
#include <QCoreApplication>
#include <QDialog>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QPainter>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <sstream>
#include <vector>

#include "arkangel_report_util.h"
#include "command_exit_codes.h"

namespace arcxpress {

FloorplanGraphicsView::FloorplanGraphicsView(QWidget* parent)
  : QGraphicsView(parent), scene_(new QGraphicsScene(this)) {
  setScene(scene_);
  setDragMode(QGraphicsView::NoDrag);
  setTransformationAnchor(QGraphicsView::AnchorViewCenter);
  setResizeAnchor(QGraphicsView::AnchorViewCenter);
  // Keep hard edges so tile gaps remain visible at any zoom.
  setRenderHint(QPainter::Antialiasing, false);
  setRenderHint(QPainter::TextAntialiasing);
  scene_->setBackgroundBrush(QBrush(Qt::white));
}

void FloorplanGraphicsView::render_layout(
  const std::vector<FloorplanGraphicsView::ShapeRect>& rects, float canvas_w, float canvas_h,
  float canvas_pad, size_t precision, const std::vector<bool>& overlap_flags) {
  rects_ = rects;
  canvas_w_ = canvas_w;
  canvas_h_ = canvas_h;
  canvas_pad_ = canvas_pad;
  precision_ = precision;
  overlap_flags_ = overlap_flags;
  redraw_scene();
}

size_t FloorplanGraphicsView::analyze_tile_geometry(
  const std::vector<FloorplanGraphicsView::ShapeRect>& rects, std::vector<bool>& is_overlapping,
  float& min_gap_x, float& min_gap_y) {
  constexpr float kGeomEps = 1e-4f;
  is_overlapping.assign(rects.size(), false);
  min_gap_x = std::numeric_limits<float>::max();
  min_gap_y = std::numeric_limits<float>::max();
  bool saw_pair = false;
  size_t overlap_pair_count = 0;

  for (size_t i = 0; i < rects.size(); ++i) {
    const auto& a = rects[i];
    if (a.module == "fpga_core" || a.w <= 0.f || a.h <= 0.f) {
      continue;
    }
    for (size_t j = i + 1; j < rects.size(); ++j) {
      const auto& b = rects[j];
      if (b.module == "fpga_core" || b.w <= 0.f || b.h <= 0.f) {
        continue;
      }
      saw_pair = true;
      if (has_positive_overlap(a, b, kGeomEps)) {
        is_overlapping[i] = true;
        is_overlapping[j] = true;
        ++overlap_pair_count;
      }

      const float a_x1 = a.x + a.w;
      const float a_y1 = a.y + a.h;
      const float b_x1 = b.x + b.w;
      const float b_y1 = b.y + b.h;
      const float gap_x = std::max(0.f, std::max(a.x, b.x) - std::min(a_x1, b_x1));
      const float gap_y = std::max(0.f, std::max(a.y, b.y) - std::min(a_y1, b_y1));
      min_gap_x = std::min(min_gap_x, gap_x);
      min_gap_y = std::min(min_gap_y, gap_y);
    }
  }

  if (!saw_pair) {
    min_gap_x = 0.f;
    min_gap_y = 0.f;
  }
  return overlap_pair_count;
}

void FloorplanGraphicsView::resizeEvent(QResizeEvent* event) {
  QGraphicsView::resizeEvent(event);
  redraw_scene();
}

bool FloorplanGraphicsView::has_positive_overlap(const FloorplanGraphicsView::ShapeRect& a,
                                                 const FloorplanGraphicsView::ShapeRect& b,
                                                 float eps) {
  const float a_x0 = a.x;
  const float a_y0 = a.y;
  const float a_x1 = a.x + a.w;
  const float a_y1 = a.y + a.h;

  const float b_x0 = b.x;
  const float b_y0 = b.y;
  const float b_x1 = b.x + b.w;
  const float b_y1 = b.y + b.h;

  const float overlap_w = std::min(a_x1, b_x1) - std::max(a_x0, b_x0);
  const float overlap_h = std::min(a_y1, b_y1) - std::max(a_y0, b_y0);
  return overlap_w > eps && overlap_h > eps;
}

void FloorplanGraphicsView::redraw_scene() {
  scene_->clear();
  scene_->setSceneRect(-canvas_pad_, -canvas_pad_, canvas_w_ + 2 * canvas_pad_,
                       canvas_h_ + 2 * canvas_pad_);

  // Always use enhanced mode: shrink each rectangle slightly so sub-pixel gaps are visible.
  const float inset = 0.4f;
  for (size_t i = 0; i < rects_.size(); ++i) {
    const auto& r = rects_[i];
    if (r.module == "fpga_core") {
      continue;
    }
    if (r.w <= 0.f || r.h <= 0.f) {
      continue;
    }
    if (r.x + r.w < 0.f || r.y + r.h < 0.f || r.x > canvas_w_ || r.y > canvas_h_) {
      continue;
    }
    const QColor tile_color =
      QColor::fromHsv(static_cast<int>(std::hash<std::string>{}(r.module) % 360), 120, 235, 210);
    const float qt_y = canvas_h_ - (r.y + r.h);
    const float draw_x = r.x + inset;
    const float draw_y = qt_y + inset;
    const float draw_w = std::max(0.1f, r.w - 2.f * inset);
    const float draw_h = std::max(0.1f, r.h - 2.f * inset);
    const QRectF draw_rect(draw_x, draw_y, draw_w, draw_h);
    const bool is_overlapping = (i < overlap_flags_.size()) ? overlap_flags_[i] : false;
    const QPen border_pen = is_overlapping ? QPen(QColor(220, 20, 60), 0.0) : Qt::NoPen;
    QGraphicsRectItem* rect = scene_->addRect(draw_rect, border_pen, QBrush(tile_color));
    rect->setAcceptHoverEvents(true);
    const std::string overlap_note = is_overlapping ? "\nStatus: OVERLAP detected" : "";
    rect->setToolTip(QString::fromStdString(
      "Tile: " + r.module + "\nInstance: " + r.instance + "\n(x, y): (" +
      to_string_with_precision(r.x, precision_) + ", " + to_string_with_precision(r.y, precision_) +
      ")\n(w, h): (" + to_string_with_precision(r.w, precision_) + ", " +
      to_string_with_precision(r.h, precision_) + ")" + overlap_note));
  }

  // Draw fpga core boundary at the bottom layer so it does not hide tiles.
  for (const auto& r : rects_) {
    if (r.module != "fpga_core") {
      continue;
    }
    const float qt_y = canvas_h_ - (r.y + r.h);
    QPen core_pen(Qt::darkGray);
    core_pen.setWidthF(0.0);
    core_pen.setCosmetic(true);
    QGraphicsRectItem* core = scene_->addRect(QRectF(r.x, qt_y, r.w, r.h), core_pen, Qt::NoBrush);
    core->setZValue(-1.0);
  }

  fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
}

static bool parse_shaping_file(const std::string& shaping_rpt,
                               std::vector<FloorplanGraphicsView::ShapeRect>& rects) {
  std::ifstream ifs(shaping_rpt);
  if (!ifs.is_open()) {
    return false;
  }

  std::string line;
  bool seen_header = false;
  while (std::getline(ifs, line)) {
    if (line.empty()) {
      continue;
    }
    std::istringstream iss(line);
    FloorplanGraphicsView::ShapeRect rect;
    std::string shape;
    int bbox_pts = 0;
    if (!(iss >> rect.instance >> rect.module >> rect.x >> rect.y >> shape >> bbox_pts)) {
      continue;
    }
    if (shape != "rect" || bbox_pts != 4) {
      continue;
    }
    float x0 = 0.f, y0 = 0.f, x1 = 0.f, y1 = 0.f, x2 = 0.f, y2 = 0.f, x3 = 0.f, y3 = 0.f;
    if (!(iss >> x0 >> y0 >> x1 >> y1 >> x2 >> y2 >> x3 >> y3)) {
      if (!seen_header) {
        // likely header line
        seen_header = true;
      }
      continue;
    }
    rect.w = std::max({x0, x1, x2, x3}) - std::min({x0, x1, x2, x3});
    rect.h = std::max({y0, y1, y2, y3}) - std::min({y0, y1, y2, y3});
    if (rect.w > 0.f && rect.h > 0.f) {
      rects.push_back(rect);
    }
  }
  return !rects.empty();
}

int show_floorplan_data_window(const std::string& shaping_rpt, const TileQor& tile_qor,
                               const float& shrink_factor, const size_t& precision) {
  int argc = 0;
  QApplication* app = qobject_cast<QApplication*>(QCoreApplication::instance());
  if (nullptr == app) {
    app = new QApplication(argc, nullptr);
  }

  QDialog dialog;
  dialog.setWindowTitle("Floorplan Physical Layout Viewer");
  dialog.resize(1280, 860);

  auto* layout = new QVBoxLayout(&dialog);

  const float efpga_h = tile_qor.efpga_height();
  const float efpga_w = tile_qor.efpga_width();
  const float efpga_area = efpga_h * efpga_w / 1e6;
  const float efpga_h_post = efpga_h * shrink_factor;
  const float efpga_w_post = efpga_w * shrink_factor;

  std::vector<FloorplanGraphicsView::ShapeRect> shape_rects;
  if (!parse_shaping_file(shaping_rpt, shape_rects)) {
    VTR_LOG_ERROR("Failed to parse shaping report: %s\n", shaping_rpt.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::vector<bool> overlap_flags;
  float min_gap_x = 0.f;
  float min_gap_y = 0.f;
  const size_t overlap_pair_count =
    FloorplanGraphicsView::analyze_tile_geometry(shape_rects, overlap_flags, min_gap_x, min_gap_y);

  std::stringstream summary_ss;
  summary_ss << "Shaping report: " << shaping_rpt << "\n"
             << "Loaded rectangles: " << shape_rects.size() << "\n"
             << "Pre-shrink H/W (um): " << to_string_with_precision(efpga_h, precision) << " / "
             << to_string_with_precision(efpga_w, precision) << "\n"
             << "Post-shrink H/W (um): " << to_string_with_precision(efpga_h_post, precision)
             << " / " << to_string_with_precision(efpga_w_post, precision) << "\n"
             << "Area (mm^2): " << to_string_with_precision(efpga_area, precision) << "\n"
             << "Horizontal/Vertical tile gap (um): "
             << to_string_with_precision(tile_qor.horizontal_tile_gap(), precision) << " / "
             << to_string_with_precision(tile_qor.vertical_tile_gap(), precision) << "\n"
             << "Observed min pairwise gap X/Y (um): "
             << to_string_with_precision(min_gap_x, precision) << " / "
             << to_string_with_precision(min_gap_y, precision) << "\n"
             << "Overlapping tile pairs (canvas check): " << overlap_pair_count << "\n"
             << "Average target/actual util (%): "
             << to_string_with_precision(100.f * tile_qor.average_target_utilization(), precision)
             << " / "
             << to_string_with_precision(100.f * tile_qor.average_actual_utilization(), precision);

  auto* summary = new QLabel(QString::fromStdString(summary_ss.str()), &dialog);
  summary->setTextInteractionFlags(Qt::TextSelectableByMouse);
  layout->addWidget(summary);

  if (shape_rects.empty()) {
    // fallback to the full-chip box to avoid an empty window
    FloorplanGraphicsView::ShapeRect fallback;
    fallback.instance = "fpga_core";
    fallback.module = "fpga_core";
    fallback.x = 0.f;
    fallback.y = 0.f;
    fallback.w = tile_qor.efpga_width();
    fallback.h = tile_qor.efpga_height();
    shape_rects.push_back(fallback);
  }

  // Use eFPGA dimensions as canonical canvas size to avoid outlier rows
  // in shaping.txt shrinking everything into a tiny center block.
  const float canvas_w = efpga_w;
  const float canvas_h = efpga_h;
  // Add a tiny view margin to avoid clipping tiles touching top/right edges.
  const float canvas_pad = std::max(20.0f, 0.02f * std::max(canvas_w, canvas_h));

  auto* view = new FloorplanGraphicsView(&dialog);
  view->render_layout(shape_rects, canvas_w, canvas_h, canvas_pad, precision, overlap_flags);
  layout->addWidget(view);

  dialog.exec();
  return openfpga::CMD_EXEC_SUCCESS;
}

}  // namespace arcxpress

#endif

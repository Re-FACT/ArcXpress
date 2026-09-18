# ArkAngel License Generation Package

## Generation

- Extract this tarball file. Ensure the version matches your ArkAngel (See the VERSION.md)

- Navigate to the extracted directory in terminal

```
tar -xvf <YOUR_TARBALL_FILENAME.tar.gz>
```

- Run the following command to generate license:

```
cd aaee_lccgen # If you are using the default name of lccgen package
make license
```

Use the following options to customize

- ``LIC_TYPE``: Can be [``eval`` | ``prototype`` | ``full`` ]. By default, it is ``eval``
- ``LIC_DURATION``: Select the valid period (unit: days) of the license to be generated. By default, it is ``30``.

You may call other commands

- Clean generated license files

```
make clean
```

- View help desk

```
make help
```

> [!NOTE]
> Please ensure you have appropriate permissions and configuration files to generate licenses.

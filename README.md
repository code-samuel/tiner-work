# Tiner Work

Tiner Work is a lightweight Human Resources Management System built in C with GTK interface

## How to Use It

### 1. Clone the repo
```bash
git clone https://github.com/code-samuel/tiner-work
cd tiner-work
```
### 2. Setup the meson build file
(Use this step only the first time you set up the project)
```bash
meson setup build
```

### 3. Compile the project
```bash
ninja -C build
```

### 4. Run the application
```bash
./build/src/rh_pim
```


### Quick command
After setting up the build directory, you can compile and run the project with a single command:

```
ninja -C build && ./build/src/rh_pim
```
## License
This project is licensed under the MIT License. See the `LICENSE` file for details.

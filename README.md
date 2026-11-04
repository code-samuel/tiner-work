# Tiner Work

Tiner Work is a lightweight Human Resources Management System built in C with GTK interface

## How to Use It
## Installation & Running Options

You can either install the pre-packaged **Flatpak** binary (recommended for end users) or build the project from source.

### Option A: Flatpak Installation (Recommended for Linux)

#### Prerequisites
Ensure `flatpak` is available on your Linux distribution:
```bash
sudo apt install flatpak    # Debian / Ubuntu
sudo dnf install flatpak    # Fedora
sudo pacman -S flatpak      # Arch Linux
```
### 1. Clone the repo
```bash
git clone https://github.com/code-samuel/tiner-work
cd tiner-work
```

### 2. Start the Database (Docker)

Download docker-compose.yml and start the MySQL/MariaDB service:

```bash 
docker compose up -d
```


### 3. Install & Run the Bundle

Download rh_pim.flatpak from the Releases section and run:

```bash
flatpak install --user -y rh_pim.flatpak
flatpak run org.gnome.RhPim
```

### Option B: Building from Source 

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

### 4. Database Setup (Docker) 
Start the MySQL database service using Docker Compose. It will automatically initialize the schema using **Database.sql**

```bash
docker compose up -d
```

### 5. Run the application
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

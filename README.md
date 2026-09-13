<p align="center">
  <img src="https://github.com/user-attachments/assets/7fe6eba8-ce21-4cc5-8d6e-a6a3db96441a" alt="QUEM" width="380">
</p>

<p align="center">
   <b>Read in:</b>
  <a href="README.md">English</a> |
  <a href="README/README-es.md">Español</a>
</p>

<p align="center">
  <b>Your TUI app to Burn operating systems to USB drives.</b>
</p>

<p align="center">
  
  <img src="https://img.shields.io/badge/Language-C-blue.svg" alt="Language">
  <img src="https://img.shields.io/badge/Interface-TUI-orange.svg" alt="Interface">
  <img src="https://img.shields.io/badge/License-MIT-green.svg" alt="License">
  <img src="https://img.shields.io/badge/Platform-Linux-yellow.svg" alt="Platform">
</p>

## Description

This project was born as an experiment to study memory management, how partition tables work, and low-level flashing systems.

During my research, I noticed the lack of a pure TUI (Terminal User Interface) alternative to classic OS burning applications for removable drives. The current ecosystem is divided into two extremes: pure terminal commands or heavy graphical applications. **QUEM** is the middle ground: it aims to provide clear visual information, completely prevent accidentally burning the wrong disks, be incredibly easy to use, and run natively in any Linux terminal and environment.

---

## Architecture and Process

QUEM's source code is divided into main modules that interact directly with the hardware:

*   **`find_os`**: Uses the POSIX function `nftw` (File Tree Walk) to scan directories for `.iso` and `.img` files. To avoid processing corrupt files or failed downloads, it automatically filters out any file smaller than 100 MB, storing the exact name, size, and location in memory.
*   **`find_usb`**: Using `dirent`, it scans the `/sys/block/` directory to identify storage devices. It reads hardware flags to check if they are removable drives (`1` for removable, `0` for internal disk), and then builds the necessary paths to extract their size and model.
*   **`burn`**: The engine of QUEM. It is a simple system that opens the ISO file and the selected USB drives, reads the image into **4 MB buffers**, and passes this block to a loop that writes simultaneously to all target drives until the file is finished.
*   **`renders`**: The visual rendering system was a great technical journey, in this case supported by AI to learn how to create a responsive engine that detects when the terminal window is resized. 
    *   *Fun fact:* The ASCII logos of the distros (and the default disk/USB icons) were generated through an auxiliary mini-program I built using `libchafa` to get the exact ASCII sequence (a project that might be refactored and published soon). QUEM's render engine scans the exact name of the ISO, and if it matches a common Linux distro, it automatically assigns its logo.

---

<p align="center">
<img width="854" height="480" alt="gifmuestra" src="https://github.com/user-attachments/assets/bf0fcda8-7229-4079-a1d3-ed720c34f8ff" />

<img width="596" height="229" alt="imagen" src="https://github.com/user-attachments/assets/f5983915-bc1f-4386-8202-671759a45c11" />


<img width="753" height="343" alt="imagen" src="https://github.com/user-attachments/assets/7090cb44-34bf-4966-bb31-a12035a6ba0d" />

<img width="512" height="220" alt="imagen" src="https://github.com/user-attachments/assets/7ae6b509-afda-4aa7-8823-6f22f2ef2941" />
</P>

---

## Usage Guide

Currently, QUEM specializes exclusively in flashing drives quickly and safely *(although I already envision a QUEM 2.0 that allows including multiple OSes on a single USB drive and other super useful features)*.

To use it, simply type in your terminal:

```bash
quem
```

The visual interface will open instantly, detecting all downloaded OSes on your PC and all connected USB drives.

---

## Installation Guide

Clone the repository and compile the project using the included Makefile:

```bash
git clone https://github.com/Neschuk/quem.git
cd quem
make
sudo make install

```
---

>[!CAUTION]
> SECURITY ALERT
> WARNING: Once the flashing process begins, all previous data on the USB drive will be permanently erased.

QUEM features a built-in safety engine: if you cancel the flashing process midway (by pressing Q), the program will not leave your USB drive corrupted. It will automatically reset the first 4 MB of the disk to zeros (0), eliminating any partially written OS instructions and leaving your USB drive completely clean, unpartitioned, and ready to be formatted and used again.

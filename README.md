# Atsumari

<img src="appicon/atsumari.svg" width=128/>

![](https://img.shields.io/github/stars/xiluembo/atsumari.svg) ![](https://img.shields.io/github/forks/xiluembo/atsumari.svg) ![](https://img.shields.io/github/tag/xiluembo/atsumari.svg) ![](https://img.shields.io/github/release/xiluembo/atsumari.svg) ![](https://img.shields.io/github/issues/xiluembo/atsumari.svg)

Atsumari is a stream overlay application designed to interact with Twitch chat and its services. The application dynamically reads emojis and emotes from the chat and renders them in real-time on the surface of a rotating sphere, providing a visually engaging and interactive experience for stream viewers.

Designed to be used with:
<img src="https://img.shields.io/badge/Twitch-9146FF?logo=twitch&logoColor=white"/>
<img src="https://img.shields.io/badge/OBS_Studio-302E31?logo=obsstudio"/>

## Building Atsumari

### Dependencies:

Atsumari is tested using the following software versions, those are not minimal requirements, lower versions may work as well, albeit untested:


| Operating System | Qt Version | CMake Version | Compiler | 
| :------------ | :------------ | :------------ | :------------ |
| <img src="https://img.shields.io/badge/Ubuntu-24.04-E95420?logo=ubuntu"/> | <img src="https://img.shields.io/badge/Qt-6.7.2-41CD52?logo=qt"/> | <img src="https://img.shields.io/badge/CMake-3.29.3-064F8C?logo=cmake"> | <img src="https://img.shields.io/badge/GCC-3.29.3-4EAA25?logo=gnu"> |
| <img src="https://img.shields.io/badge/Ubuntu-24.04-E95420?logo=ubuntu"/> | <img src="https://img.shields.io/badge/Qt-6.7.2-41CD52?logo=qt"/> | <img src="https://img.shields.io/badge/CMake-3.29.3-064F8C?logo=cmake"> | <img src="https://img.shields.io/badge/Clang-14.0.0-4EAA25?logo=LLVM"> |
| <img src="https://img.shields.io/badge/Windows-11-0078d4?logo=data:image/svg%2bxml;base64,PHN2ZyByb2xlPSJpbWciIHZpZXdCb3g9IjAgMCAyNCAyNCIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj48dGl0bGU+V2luZG93cyAxMTwvdGl0bGU+PHBhdGggZD0iTTAsMEgxMS4zNzdWMTEuMzcySDBaTTEyLjYyMywwSDI0VjExLjM3MkgxMi42MjNaTTAsMTIuNjIzSDExLjM3N1YyNEgwWm0xMi42MjMsMEgyNFYyNEgxMi42MjMiLz48L3N2Zz4="/> | <img src="https://img.shields.io/badge/Qt-6.7.2-41CD52?logo=qt"/> | <img src="https://img.shields.io/badge/CMake-3.29.3-064F8C?logo=cmake"> | <img src="https://img.shields.io/badge/MSVC-2022_community-4EAA25?logoColor=white&logo=data:image/svg%2bxml;base64,PHN2ZyByb2xlPSJpbWciIHZpZXdCb3g9IjAgMCAyNCAyNCIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj48dGl0bGU+VmlzdWFsIFN0dWRpbzwvdGl0bGU+PHBhdGggZD0iTTE3LjU4My4wNjNhMS41IDEuNSAwIDAwLTEuMDMyLjM5MiAxLjUgMS41IDAgMDAtLjAwMSAwQS44OC44OCAwIDAwMTYuNS41TDguNTI4IDkuMzE2IDMuODc1IDUuNWwtLjQwNy0uMzVhMSAxIDAgMDAtMS4wMjQtLjE1NCAxIDEgMCAwMC0uMDEyLjAwNWwtMS44MTcuNzVhMSAxIDAgMDAtLjA3Ny4wMzYgMSAxIDAgMDAtLjA0Ny4wMjggMSAxIDAgMDAtLjAzOC4wMjIgMSAxIDAgMDAtLjA0OC4wMzQgMSAxIDAgMDAtLjAzLjAyNCAxIDEgMCAwMC0uMDQ0LjAzNiAxIDEgMCAwMC0uMDM2LjAzMyAxIDEgMCAwMC0uMDMyLjAzNSAxIDEgMCAwMC0uMDMzLjAzOCAxIDEgMCAwMC0uMDM1LjA0NCAxIDEgMCAwMC0uMDI0LjAzNCAxIDEgMCAwMC0uMDMyLjA1IDEgMSAwIDAwLS4wMi4wMzUgMSAxIDAgMDAtLjAyNC4wNSAxIDEgMCAwMC0uMDIuMDQ1IDEgMSAwIDAwLS4wMTYuMDQ0IDEgMSAwIDAwLS4wMTYuMDQ3IDEgMSAwIDAwLS4wMTUuMDU1IDEgMSAwIDAwLS4wMS4wNCAxIDEgMCAwMC0uMDA4LjA1NCAxIDEgMCAwMC0uMDA2LjA1QTEgMSAwIDAwMCA2LjY2OHYxMC42NjZhMSAxIDAgMDAuNjE1LjkxN2wxLjgxNy43NjRhMSAxIDAgMDAxLjAzNS0uMTY0bC40MDgtLjM1IDQuNjUzLTMuODE1IDcuOTczIDguODE1YTEuNSAxLjUgMCAwMC4wNzIuMDY1IDEuNSAxLjUgMCAwMC4wNTcuMDUgMS41IDEuNSAwIDAwLjA1OC4wNDIgMS41IDEuNSAwIDAwLjA2My4wNDQgMS41IDEuNSAwIDAwLjA2NS4wMzggMS41IDEuNSAwIDAwLjA2NS4wMzYgMS41IDEuNSAwIDAwLjA2OC4wMzEgMS41IDEuNSAwIDAwLjA3LjAzIDEuNSAxLjUgMCAwMC4wNzMuMDI1IDEuNSAxLjUgMCAwMC4wNjYuMDIgMS41IDEuNSAwIDAwLjA4LjAyIDEuNSAxLjUgMCAwMC4wNjguMDE0IDEuNSAxLjUgMCAwMC4wNzUuMDEgMS41IDEuNSAwIDAwLjA3NS4wMDggMS41IDEuNSAwIDAwLjA3My4wMDMgMS41IDEuNSAwIDAwLjA3NyAwIDEuNSAxLjUgMCAwMC4wNzgtLjAwNSAxLjUgMS41IDAgMDAuMDY3LS4wMDcgMS41IDEuNSAwIDAwLjA4Ny0uMDE1IDEuNSAxLjUgMCAwMC4wNi0uMDEyIDEuNSAxLjUgMCAwMC4wOC0uMDIyIDEuNSAxLjUgMCAwMC4wNjgtLjAyIDEuNSAxLjUgMCAwMC4wNy0uMDI4IDEuNSAxLjUgMCAwMC4wOS0uMDM3bDQuOTQ0LTIuMzc3YTEuNSAxLjUgMCAwMC40NzYtLjM2MiAxLjUgMS41IDAgMDAuMDktLjExMiAxLjUgMS41IDAgMDAuMDA0LS4wMDcgMS41IDEuNSAwIDAwLjA4LS4xMjUgMS41IDEuNSAwIDAwLjA2Mi0uMTIgMS41IDEuNSAwIDAwLjAwOS0uMDE3IDEuNSAxLjUgMCAwMC4wNC0uMTA4IDEuNSAxLjUgMCAwMC4wMTUtLjAzNyAxLjUgMS41IDAgMDAuMDMtLjEwNyAxLjUgMS41IDAgMDAuMDA5LS4wMzcgMS41IDEuNSAwIDAwLjAxNy0uMSAxLjUgMS41IDAgMDAuMDA4LS4wNSAxLjUgMS41IDAgMDAuMDA2LS4wOSAxLjUgMS41IDAgMDAuMDA0LS4wOFYzLjk0MmExLjUgMS41IDAgMDAwLS4wMDMgMS41IDEuNSAwIDAwMC0uMDMyIDEuNSAxLjUgMCAwMC0uMDEtLjE1IDEuNSAxLjUgMCAwMC0uODQtMS4xN0wxOC4yMDYuMjFhMS41IDEuNSAwIDAwLS42MjItLjE0NnpNMTggNi45MnYxMC4xNjNsLTYuMTk4LTUuMDh6TTMgOC41NzRsMy4wOTkgMy40MjctMy4xIDMuNDI2eiIvPjwvc3ZnPg=="/>  |

### Steps:

1. Clone the repository:
    ```bash
    git clone https://github.com/xiluembo/atsumari.git
    ```

2. Build the application using CMake:
    ```bash
    cd atsumari
    mkdir build
    cd build
    cmake ..
    cmake --build .
    ```
    Alternatively, open the CMakeLists.txt file from Qt Creator as a project, and build using Qt Creator IDE.

## Running the application:

Simply run `atsumari` (or `atsumari.exe` on Windows) from the application folder.

After configuring your preferences and hitting the "Close this Dialog and Run" button, the main Atsumari window will appear, and an additional Twitch prompt may appear on the first run, or after the authentication token is expired.

You can then add the Atsumari window as a source to your broadcasting software (like OBS Studio). It is recommended to use a "Color Key" filter with the source, specifying the color black, in order to remove the black background.

## Contributing:

### Translations:

To add a translation, create a fork of this repository, then inside the fork update the CMakeLists.txt file with the desired language, by updating the following line:

```cmake
qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES pt_BR)
```

Then, inside the build directory, run:

```bash
cmake --build . --target=update_translations
```

A file named after the language identifier will be placed under the i18n folder. This file can be edited using Qt Linguist. After all strings are updated and validated, you can build the project once again and verify the new language is displayed correctly. Then, submit a merge request to have the language file added to the project.




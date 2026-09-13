<p align="center">
  <img src="https://github.com/user-attachments/assets/7fe6eba8-ce21-4cc5-8d6e-a6a3db96441a" alt="QUEM" width="380">
</p>

<p align="center">
   <b>Leer en:</b>
  <a href="../README.md">English</a> |
  <a href="/README/README-es.md">Español</a>
</p>

<p align="center">
  <b>Tu app TUI para Quem-ar sistemas operativos en unidades USB.</b>
</p>

<p align="center">
  
  <img src="https://img.shields.io/badge/Lenguaje-C-blue.svg" alt="Language">
  <img src="https://img.shields.io/badge/Interfaz-TUI-orange.svg" alt="Interface">
  <img src="https://img.shields.io/badge/Licencia-MIT-green.svg" alt="License">
  <img src="https://img.shields.io/badge/Plataforma-Linux-yellow.svg" alt="Platform">
</p>

## Descripción

Este proyecto nació como un experimento para estudiar la gestión de memoria, el funcionamiento de las tablas de particiones y los sistemas de flasheo a bajo nivel.

Durante la investigación, noté la inexistencia de una alternativa TUI (Interfaz de Usuario de Terminal) pura frente a las aplicaciones clásicas de quemado de SO en unidades extraíbles. El ecosistema actual se divide en dos extremos: comandos puros en la terminal, o aplicaciones gráficas pesadas. **QUEM** es el punto medio: busca brindar información visual clara, evitar por completo quemar discos por error, ser increíblemente fácil de usar y funcionar de forma nativa en cualquier terminal y entorno Linux.

---

## Arquitectura y Proceso

El código fuente de QUEM está dividido en módulos principales que interactúan directamente con el hardware:

*   **`find_os`**: Utiliza la función POSIX `nftw` (File Tree Walk) para escanear directorios en busca de archivos `.iso` y `.img`. Para evitar procesar archivos corruptos o descargas fallidas, filtra automáticamente todo archivo menor a 100 MB, guardando el nombre, tamaño y ubicación exacta en memoria.
*   **`find_usb`**: Utilizando `dirent`, escanea el directorio `/sys/block/` para identificar dispositivos de almacenamiento. Lee los flags del hardware para comparar si son unidades extraíbles (`1` para removible, `0` para disco interno), y luego construye las rutas necesarias para extraer el tamaño y modelo.
*   **`burn`**: El motor de QUEM. Es un sistema simple que abre el archivo ISO y los pendrives seleccionados, lee la imagen en **buffers de 4 MB**, y transmite este bloque a un bucle que escribe simultáneamente en todos los pendrives objetivo hasta finalizar el archivo.
*   **`renders`**: El sistema de renderizado visual fue una gran travesía técnica, en este caso apoyada por IA aprender a crear un motor responsivo que detecta cuándo la terminal se agranda o se achica. 
    *   *Dato curioso:* Los logotipos ASCII de las distros (y los iconos default de discos/pendrives) fueron generados a través de un mini-programa auxiliar que construí utilizando `libchafa` para obtener la secuencia ASCII exacta (proyecto que podría ser reformado y publicado pronto). El render de QUEM escanea el nombre exacto de la ISO, y si coincide con una distro común de Linux, le asigna automáticamente su logotipo.

---

<p align="center">
<img width="854" height="480" alt="gifmuestra" src="https://github.com/user-attachments/assets/bf0fcda8-7229-4079-a1d3-ed720c34f8ff" />

<img width="596" height="229" alt="imagen" src="https://github.com/user-attachments/assets/f5983915-bc1f-4386-8202-671759a45c11" />


<img width="753" height="343" alt="imagen" src="https://github.com/user-attachments/assets/7090cb44-34bf-4966-bb31-a12035a6ba0d" />

<img width="512" height="220" alt="imagen" src="https://github.com/user-attachments/assets/7ae6b509-afda-4aa7-8823-6f22f2ef2941" />
</P>

---

## Guía de Uso

Actualmente, QUEM se especializa exclusivamente en flashear discos de forma rápida y segura *(aunque ya imagino un QUEM 2.0 que permita incluir varios OS en un pendrive y otras funciones súper útiles)*.

Para usarlo, simplemente escribe en tu terminal:

```bash
quem
```
Se abrirá instantáneamente la interfaz visual detectando todos los SO descargados en tu PC y todos los pendrives conectados.

---

## Guía de Instalación

Clona el repositorio y compila el proyecto usando el `Makefile` incluido:

```bash
git clone https://github.com/Neschuk/quem.git
cd quem
make
sudo make install

```
---
> [!CAUTION]
> ALERTA DE SEGURIDAD
> ATENCIÓN: Una vez que comience el proceso de flasheo, todos los datos previos del pendrive se borrarán permanentemente.

QUEM cuenta con un motor de seguridad integrado: si cancelas el proceso de flasheo a la mitad (presionando Q), el programa no dejará tu pendrive corrupto. Automáticamente reseteará los primeros 4 MB del disco a ceros (0), eliminando cualquier instrucción a medio escribir del SO y dejando tu pendrive completamente limpio, desparticionado y listo para ser formateado y usado nuevamente.


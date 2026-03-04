# Multicore SNN Accelerator

FPGA-based multicore spiking neural network accelerator using ODIN neuromorphic cores on Zynq UltraScale+.

ODIN: https://github.com/ChFrenkel/ODIN

This project was developed as an undergraduate engineering thesis in computer science (Bachelor's degree project)
and received a grade of 5.0.

## What Is In This Repo

- `pl/` FPGA design (Vivado projects, IP, RTL)
- `ps/` ARM server software (FreeRTOS + lwIP)
- `py/` Python training scripts and server client
- `fbs/` FlatBuffers communication schema

## Images

PS software schematic

<p align="center">
	<img src="docs/images/server.svg" alt="PS software schematic" style="width:90%;height:auto;">
</p>

5 cores implemented on FPGA connected to ARM processor

<p align="center">
	<img src="docs/images/main.svg" alt="5-core FPGA and ARM Vivado block diagram" style="width:100%;height:auto;">
</p>

ODIN_CORE IP block diagram

<p align="center">
	<img src="docs/images/ODIN_CORE.svg" alt="ODIN_CORE Vivado block diagram" style="width:100%;height:auto;">
</p>

Project running on MYIR FZ3 board

<p align="center">
	<img src="docs/images/photo.jpg" alt="Project on MYIR FZ3 board" style="width:80%;height:auto;">
</p>
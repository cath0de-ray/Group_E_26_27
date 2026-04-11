# Project VITALIS: AI-Driven Stethoscope Attachment
**Versatile Intelligent Thoracic Acoustic Lung Inference System**

![Project Status](https://img.shields.io/badge/Status-Prototype_Development-blue)
![AI Accuracy](https://img.shields.io/badge/AI_Accuracy-80.7%25-green)
![Grade](https://img.shields.io/badge/Academic_Level-12th_Grade-purple)
![License](https://img.shields.io/badge/License-CC_BY--NC--SA_4.0-lightgrey)

## 👥 Development Team
* **Pradhumya Singh Yadav**
* **Dhairya Chaudhary**
* **Aryan Chauhan**

---

## 🩺 Executive Summary
**VITALIS** (Versatile Intelligent Thoracic Acoustic Lung Inference System) is an ultra-low-cost, offline Artificial Intelligence diagnostic tool. It is engineered as a modular attachment that retrofits onto any standard analog stethoscope. 

By capturing acoustic pulmonary data and processing it locally via Edge-AI, VITALIS identifies respiratory anomalies (Wheezes, Crackles, Rhonchi) in real-time. It is specifically designed to empower NGO field workers and frontline health staff in resource-depleted environments to perform rapid triage without requiring an internet connection or an on-site pulmonologist.

---

## 📂 Hardware Support & Versions
Project VITALIS currently supports two different microcontroller architectures. Please navigate to the appropriate folder for your specific hardware:

* **[Hardware V2 (XIAO ESP32-C6)](/Arduino%20Code/XIAO%20ESP32C6):** The primary miniaturized commercial prototype. Utilizes a 160MHz RISC-V architecture optimized for extreme low-latency edge inference (Inference time: 6ms). *[C++ Implementation Pending]*
* **[Hardware V1 (Classic ESP32)](/Arduino%20Code/ESP-32-WROOM):** The original baseline model utilizing the 240MHz dual-core Xtensa architecture. Ideal for legacy environments and standard ESP32 DevKits.

---

## 🌍 Alignment with UN Sustainable Development Goals (SDGs)
This project is purpose-built to address the "Last Mile" healthcare gap, actively targeting three primary UN SDGs:

### 1. SDG 3: Good Health and Well-being
* **The Problem:** Respiratory diseases are leading causes of mortality in rural areas due to misdiagnosis.
* **The VITALIS Solution:** Providing an automated "second opinion" for lung auscultation to facilitate early detection and timely intervention.

### 2. SDG 9: Industry, Innovation, and Infrastructure
* **The Problem:** Commercial digital stethoscopes are prohibitively expensive and rely on cloud computing.
* **The VITALIS Solution:** Utilizes decentralized "Edge Computing" via quantized int8 neural networks to provide cutting-edge diagnostic infrastructure with zero internet dependency.

### 3. SDG 10: Reduced Inequalities
* **The Problem:** Severe disparity in diagnostic quality between urban hospitals and rural clinics.
* **The VITALIS Solution:** Democratizes advanced medical technology by upgrading standard analog equipment into digital diagnostic nodes using affordable MEMS microphones.

---

## ⚙️ System Architecture & Workflow
VITALIS operates on a completely localized, closed-loop pipeline ensuring 100% patient data privacy.

1. **Acoustic Coupling:** Analog lung sounds are captured via the stethoscope chest-piece and routed through a modular T-junction.
2. **Digital Conversion:** The **INMP441 I2S MEMS Microphone** samples the audio at high fidelity, converting acoustic waves into digital I2S data streams.
3. **DSP & Feature Extraction:** The MCU buffers audio in 1000ms windows and calculates Mel Frequency Cepstral Coefficients (MFCC) to isolate acoustic "fingerprints."
4. **AI Inference:** The extracted features are fed into the quantized EON-compiled neural network.
5. **Output UI:** The predicted respiratory state is immediately displayed on the **OLED**, providing clear triage instructions to the operator.

---

## 🧠 Neural Network Performance
The core inference engine was trained via Edge Impulse on a clinically verified dataset and cross-profiled for both Xtensa and RISC-V architectures.

### Validation Metrics
* **Overall Accuracy:** 80.7%
* **Overall F1 Score:** 0.80
* **Training Depth:** 600 Epochs
* **Optimization:** Quantized (int8) to operate within the SRAM constraints of the ESP32 ecosystem.

### Evidence of Training & Hardware Optimization

#### **Hardware V2: Seeed Studio XIAO ESP32-C6 (160MHz RISC-V)**
*Extreme optimization via EON™ Compiler, achieving a 6ms inference latency footprint.*
![XIAO C6 Accuracy Report](accuracy_v2_c6.png)

#### **Hardware V1: Classic ESP32 (240MHz Xtensa)**
*The original baseline model.*
![Classic ESP32 Accuracy Report](accuracy_report.png)

---

## 🛠️ Hardware Specifications

### **Prototype A**
* **MCU:** Seeed Studio XIAO ESP32-C6 (Single-Core RISC-V @ 160MHz)
* **Acoustic Sensor:** INMP441 I2S Digital MEMS Microphone
* **Display Interface:** 0.96" I2C OLED (SSD1306)

### **Prototype B**
* **MCU:** ESP32 DevKit V1 (Dual-Core Xtensa LX6 @ 240MHz)
* **Acoustic Sensor:** INMP441 I2S Digital MEMS Microphone
* **Display Interface:** 0.96" I2C OLED (SSD1306)

---

## ⚖️ Intellectual Property & Licensing

### **Non-Commercial License**
This project is licensed under the **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)**.
Under this license, others may share and adapt this material with proper **Attribution**, ensuring **Non-Commercial** use, and distributing modifications under the same **ShareAlike** license.

### **Patent & Prior Art Notice**
The Lead Developer reserves the right to file for **Patent Protection** regarding the unique hardware integration, acoustic coupling mechanism, and neural network architecture. This public disclosure serves as **Prior Art** under international patent law.

---

## 📂 Dataset Attribution
The AI model was trained using the HLS-CMDS dataset.
> **Citation:** Y. Torabi, S. Shirani and J. P. Reilly, "Descriptor: Heart and Lung Sounds Dataset Recorded from a Clinical Manikin using Digital Stethoscope (HLS-CMDS)," in IEEE Data Descriptions, 2025. © 2024 Yasaman Torabi.

---
**Institution:** Jagran Public School, Noida  
**Submission:** CBSE Class 12 Artificial Intelligence Practical (2026-27)
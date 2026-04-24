# Instrument Matériel Polyrythmique

> Un moteur audio modulaire haute performance conçu pour les instruments matériels de musique électronique.

## 🎹 Vue d'ensemble

**Le Firmware** est un système audio modulaire C++20 conçu pour les instruments matériels polyrythmiques. Il offre un moteur audio haute performance (Linux/ALSA), une architecture modulaire extensible, et une interface utilisateur (graphique LVGL).

Le projet facilite la création d'instruments de musique électronique personnalisés avec contrôle matériel, synthèse modulaire, et traitement du signal audio en temps réel.

## ✨ Caractéristiques principales

### Moteur Audio
- **Architecture modulaire** : Système de modules interchangeables (oscillateurs, filtres, séquenceurs, samplers)
- **Traitement graphe audio** : Routage flexible des signaux entre les modules
- **Gestion polyphonique** : Support multi-voix avec gestion intelligente des voix
- **Synthèse en temps réel** : Génération et traitement du signal avec latence minimale

### Interface Utilisateur
- **LVGL** : Interface graphique moderne et réactive

### Plateforme & Performance
- 🖥️ **Plateforme** : Linux (ALSA)
- ⚡ **Haute performance** : Optimisé pour la latence faible
- 🔌 **Communication IPC** : Mémoire partagée pour contrôle matériel


## 🏗️ Architecture

```
┌─────────────────────────────────────────────────┐
│              Application (App)                   │
├─────────────────────────────────────────────────┤
│                                                  │
│  ┌───────────────────────────────────────────┐  │
│  │          Audio Engine                      │  │
│  │  ┌──────────────────────────────────────┐ │  │
│  │  │       Audio Graph                    │ │  │
│  │  │  ┌─ Module ─ Port ─ Signal ─┐       │ │  │
│  │  │  │      ↓       ↓      ↓      │       │ │  │
│  │  │  │   Osc → Filter → Out      │       │ │  │
│  │  │  └──────────────────────────┘       │ │  │
│  │  │                                      │ │  │
│  │  │  Voice Manager (Polyphonie)         │ │  │
│  │  └──────────────────────────────────────┘ │  │
│  └───────────────────────────────────────────┘  │
│                    ↕↕                           │
│  ┌───────────────────────────────────────────┐  │
│  │              GUI Layer                     │  │
│  │  └─ LVGL (Graphique)                      │  │
│  └───────────────────────────────────────────┘  │
│                                                  │
└─────────────────────────────────────────────────┘
          ↕ Communication IPC (HardwareInputService)
         Matériel / Entrées externes
```

### Couches principales

- **Engine Core** : Gestion du moteur audio, VoiceManager, paramètres
- **Engine Modules** : Modules audio (oscillateurs, filtres, séquenceurs, samplers)
- **Engine Ports** : Système de ports pour le routage des signaux
- **Audio Graph** : Graphe de modules interconnectés
- **GUI** : Interfaces utilisateur (LVGL)
- **IPC** : Communication avec le matériel (SharedMemory)

## 🚀 Démarrage rapide

### Prérequis

#### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libalsa-ocaml-dev \
    libsndfile1-dev \
    libsdl2-dev \
    libdrm-dev
```

### Installation et compilation

1. **Cloner le projet**
```bash
git clone https://github.com/polyrythm-hardware-instrument.git
cd polyrythm-hardware-instrument
```

2. **Créer et configurer le build**
```bash
mkdir -p build
cd build
cmake ..
```

3. **Compiler**
```bash
cmake --build . -j$(nproc)
```

4. **Lancer l'application**
```bash
./TinyFirmware
```

### Options de compilation

```bash
# Avec interface graphique LVGL
cmake ..
```

## 📖 Utilisation

### Mode interactif
```bash
./TinyFirmware
# L'interface graphique LVGL se lance automatiquement
```

## ➰ Câblage

### MCP3008 -> Raspberry Pi (SPI0 CE0)

- MCP3008 `VDD` (pin 16) -> Pi `3V3` (pin 1)
- MCP3008 `VREF` (pin 15) -> Pi `3V3` (pin 1)
- MCP3008 `AGND` (pin 14) -> Pi `GND` (pin 6)
- MCP3008 `DGND` (pin 9)  -> Pi `GND` (pin 6)
- MCP3008 `CLK` (pin 13)  -> Pi `GPIO11/SCLK` (pin 23)
- MCP3008 `DOUT` (pin 12) -> Pi `GPIO9/MISO` (pin 21)
- MCP3008 `DIN` (pin 11)  -> Pi `GPIO10/MOSI` (pin 19)
- MCP3008 `CS` (pin 10)   -> Pi `GPIO8/CE0` (pin 24)

Câblage des potentiomètres (pour chaque canal CH0..CH4):
- Extrémité A du potentiomètre -> 3V3
- Extrémité B du potentiomètre -> GND
- Curseur (broche centrale) -> MCP3008 `CHx`

### MCP23017 -> Raspberry Pi (I2C1)

- MCP23017 `VDD` -> Pi `3V3` (pin 1)
- MCP23017 `VSS` -> Pi `GND` (pin 6)
- MCP23017 `SDA` -> Pi `GPIO2/SDA1` (pin 3)
- MCP23017 `SCL` -> Pi `GPIO3/SCL1` (pin 5)
- MCP23017 `RESET` -> 3V3
- MCP23017 `A0`, `A1`, `A2` -> GND (I2C address `0x20`)

Boutons sur le MCP23017 :
- Un côté de chaque bouton -> GND
- L'autre côté -> `GPA0..GPA7` et `GPB0..GPB7`

### KY-040 -> Raspberry Pi (GPIO directs)

- KY-040 `+` -> Pi `3V3` (pin 17)
- KY-040 `GND` -> Pi `GND` (pin 9)
- KY-040 `CLK` -> Pi `GPIO17` (pin 11)
- KY-040 `DT` -> Pi `GPIO27` (pin 13)
- KY-040 `SW` -> Pi `GPIO22` (pin 15)

## 📚 Ressources supplémentaires

- [Documentation Doxygen](docs/doxygen/mainPage.md) : API complète et architecture
- [Architecture v2](docs/diagrams/architecturev2.puml) : Diagrammes architecture

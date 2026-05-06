

# 📻 M5StickRadio
<img width="960" height="740" alt="photo_2026-05-06_20-33-46" src="https://github.com/user-attachments/assets/b7867a8b-c21e-445b-88a1-893ae6c0cc97" />

> 🇬🇧 [English](#english) &nbsp;|&nbsp; 🇷🇺 [Русский](#русский)

---

<a name="english"></a>
## 🇬🇧 English

Internet radio player for **M5StickCPlus2** (ESP32). Streams MP3/AAC stations over Wi-Fi with physical button controls and a browser-based web interface.

### Features

- Internet radio streaming (MP3 / AAC / .m3u / .pls)
- 10 built-in stations
- Add and delete custom stations via browser
- Wi-Fi setup via captive portal (no PC needed)
- Web UI: playback control, direct station selection by click
- Stream title display (ICY metadata)
- Animated spectrum visualizer
- Volume control (0–21)

### Hardware

| Component | Description |
|-----------|-------------|
| **M5StickCPlus2** | ESP32 · 240 MHz · 240×135 TFT |
| **PCM5102 DAC module** | I2S audio DAC — required for sound output |
| Button **A** | Next station / volume up |
| Button **B** | Previous station / volume down |
| I2S BCLK | GPIO 26 → PCM5102 BCK |
| I2S LRC  | GPIO 0  → PCM5102 LCK |
| I2S DOUT | GPIO 25 → PCM5102 DIN |

### Button Controls

| Action | Result |
|--------|--------|
| A — single click | Next station |
| B — click | Previous station |
| A — double click | Enter volume mode |
| A (volume mode) | Volume + |
| B (volume mode) | Volume − |
| 3 s idle | Exit volume mode |

### Architecture

```
┌─────────────────────────────────────────────────────────┐
│                      ESP32 FreeRTOS                     │
│                                                         │
│  Core 0 — audioTask (priority 2)                        │
│  ┌──────────────────────────────────────────────────┐   │
│  │  xQueueReceive → audio.connecttohost(url)        │   │
│  │  audio.loop() → audio.isRunning() → isPlaying   │   │
│  │  audio_showstreamtitle() → streamTitle[]         │   │
│  └──────────────────────────────────────────────────┘   │
│            ↕ stationQueue (depth 1, overwrite)          │
│  Core 1 — loop() (priority 1)                           │
│  ┌──────────────────────────────────────────────────┐   │
│  │  handleButtons() → tickSpectrum()                │   │
│  │  webServerHandle() → if uiDirty: drawUI()        │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

**Shared state (volatile):** `currentStation`, `volume`, `volumeMode`, `isPlaying`, `uiDirty`

| File | Purpose |
|------|---------|
| `sketch_apr28a.ino` | Entry point, buttons, RTOS, audio callbacks |
| `wifi_manager.h` | Wi-Fi connection, captive portal setup |
| `station_store.h` | Station storage (built-in + custom, NVS) |
| `web_server.h` | HTTP management server on port 80 |
| `ui.h` | TFT display rendering, spectrum animation |
| `stations.h` | Built-in station list |

### Web Interface

Open in browser: `http://<device-IP>`  
The IP address is shown on the device screen after connecting to Wi-Fi.

| Endpoint | Method | Action |
|----------|--------|--------|
| `/` | GET | Management page |
| `/select` | POST | **Switch to station by index (▶ button in list)** |
| `/next` | POST | Next station |
| `/prev` | POST | Previous station |
| `/play` | POST | Resume playback |
| `/stop` | POST | Stop playback |
| `/add` | POST | Add custom station |
| `/delete` | POST | Delete station |
| `/restore` | POST | Restore built-in stations |

### Station Storage (NVS)

- **Built-in** (`stations.h`) — stored in flash, removed via bitmask `_delMask[]`
- **Custom** — up to 30 stations, persisted in NVS (`namespace: radio_st`)
- `getStation(idx)` — virtual list: non-deleted built-ins first, then custom stations

### First Boot

1. Flash the device
2. On first start the screen shows **«WiFi Setup»**
3. Connect your phone to the **RadioSetup** network (no password)
4. Open `192.168.4.1` in a browser, select your Wi-Fi network and enter the password
5. The device reboots and connects to Wi-Fi
6. The device IP appears on screen — open it in a browser to control the radio

### Dependencies

- [ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S)
- [M5StickCPlus2](https://github.com/m5stack/M5StickC-Plus2)

---

<a name="русский"></a>
## 🇷🇺 Русский

Интернет-радио на базе **M5StickCPlus2** (ESP32). Стриминг MP3/AAC станций по Wi-Fi с управлением через физические кнопки и веб-интерфейс в браузере.

### Возможности

- Стриминг интернет-радио (MP3 / AAC / .m3u / .pls)
- 10 встроенных станций
- Добавление и удаление пользовательских станций через браузер
- Настройка Wi-Fi через captive-portal (без компьютера)
- Веб-интерфейс: переключение, выбор станции одним кликом, управление воспроизведением
- Отображение названия текущего трека (ICY-метаданные)
- Анимированный спектр-индикатор
- Регулировка громкости (0–21)

### Железо

| Компонент | Описание |
|-----------|----------|
| **M5StickCPlus2** | ESP32 · 240 МГц · 240×135 TFT |
| **Модуль PCM5102 DAC** | I2S ЦАП — необходим для вывода звука |
| Кнопка **A** | Следующая станция / громкость + |
| Кнопка **B** | Предыдущая станция / громкость − |
| I2S BCLK | GPIO 26 → PCM5102 BCK |
| I2S LRC  | GPIO 0  → PCM5102 LCK |
| I2S DOUT | GPIO 25 → PCM5102 DIN |

### Управление кнопками

| Действие | Результат |
|----------|-----------|
| A — одиночный клик | Следующая станция |
| B — клик | Предыдущая станция |
| A — двойной клик | Войти в режим громкости |
| A (режим громкости) | Громкость + |
| B (режим громкости) | Громкость − |
| 3 сек без нажатий | Выйти из режима громкости |

### Архитектура

```
┌─────────────────────────────────────────────────────────┐
│                      ESP32 FreeRTOS                     │
│                                                         │
│  Core 0 — audioTask (priority 2)                        │
│  ┌──────────────────────────────────────────────────┐   │
│  │  xQueueReceive → audio.connecttohost(url)        │   │
│  │  audio.loop() → audio.isRunning() → isPlaying   │   │
│  │  audio_showstreamtitle() → streamTitle[]         │   │
│  └──────────────────────────────────────────────────┘   │
│            ↕ stationQueue (глубина 1, overwrite)        │
│  Core 1 — loop() (priority 1)                           │
│  ┌──────────────────────────────────────────────────┐   │
│  │  handleButtons() → tickSpectrum()                │   │
│  │  webServerHandle() → if uiDirty: drawUI()        │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

**Shared state (volatile):** `currentStation`, `volume`, `volumeMode`, `isPlaying`, `uiDirty`

| Файл | Назначение |
|------|-----------|
| `sketch_apr28a.ino` | Точка входа, кнопки, RTOS, audio-callbacks |
| `wifi_manager.h` | Подключение к Wi-Fi, captive-portal настройка |
| `station_store.h` | Хранилище станций (встроенные + пользовательские, NVS) |
| `web_server.h` | HTTP-сервер управления на порту 80 |
| `ui.h` | Отрисовка TFT-дисплея, анимация спектра |
| `stations.h` | Список встроенных станций |

### Веб-интерфейс

Открыть в браузере: `http://<IP-устройства>`  
IP отображается на экране устройства после подключения к Wi-Fi.

| Эндпоинт | Метод | Действие |
|----------|-------|----------|
| `/` | GET | Страница управления |
| `/select` | POST | **Переключить на станцию по индексу (клик ▶ в списке)** |
| `/next` | POST | Следующая станция |
| `/prev` | POST | Предыдущая станция |
| `/play` | POST | Воспроизвести |
| `/stop` | POST | Остановить |
| `/add` | POST | Добавить пользовательскую станцию |
| `/delete` | POST | Удалить станцию |
| `/restore` | POST | Восстановить встроенные станции |

### Хранилище станций (NVS)

- **Встроенные** (`stations.h`) — хранятся во flash, удаляются через битовую маску `_delMask[]`
- **Пользовательские** — до 30 станций, сохраняются в NVS (`namespace: radio_st`)
- `getStation(idx)` — виртуальный список: сначала не-удалённые встроенные, затем пользовательские

### Первый запуск

1. Прошить устройство
2. При первом старте на экране появится **«WiFi Setup»**
3. Подключиться телефоном к сети **RadioSetup** (без пароля)
4. Открыть `192.168.4.1` в браузере, выбрать свою Wi-Fi сеть и ввести пароль
5. Устройство перезагрузится и подключится к Wi-Fi
6. IP-адрес устройства отобразится на экране — открыть в браузере для управления

### Зависимости

- [ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S)
- [M5StickCPlus2](https://github.com/m5stack/M5StickC-Plus2)

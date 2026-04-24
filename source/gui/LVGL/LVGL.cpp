#include "gui/LVGL/LVGL.hpp"
#include "gui/LVGL/PolyRhythmCarousel.hpp"

#include <iostream>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <sys/wait.h>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <cctype>

namespace {
static const char * getenv_or_default(const char * name, const char * fallback)
{
    const char * value = std::getenv(name);
    return (value && value[0] != '\0') ? value : fallback;
}
}

#if LV_USE_LINUX_FBDEV
static void lv_linux_disp_init(void)
{
    const char *device = getenv_or_default("LV_LINUX_FBDEV_DEVICE", "/dev/fb0");
    lv_display_t * disp = lv_linux_fbdev_create();

#if LV_USE_EVDEV
    const char *evdev = getenv_or_default("LV_LINUX_EVDEV_POINTER_DEVICE", "/dev/input/event0");
    lv_indev_t * indev = lv_evdev_create(LV_INDEV_TYPE_POINTER, evdev);
    if(indev) lv_indev_set_display(indev, disp);
#endif

    lv_linux_fbdev_set_file(disp, device);
}
#elif LV_USE_LINUX_DRM
static void lv_linux_disp_init(void)
{
    const char *device = getenv_or_default("LV_LINUX_DRM_CARD", "/dev/dri/card0");
    lv_display_t * disp = lv_linux_drm_create();

#if LV_USE_EVDEV
    const char *evdev = getenv_or_default("LV_LINUX_EVDEV_POINTER_DEVICE", "/dev/input/event0");
    lv_indev_t * indev = lv_evdev_create(LV_INDEV_TYPE_POINTER, evdev);
    if(indev) lv_indev_set_display(indev, disp);
#endif

    lv_linux_drm_set_file(disp, device, -1);
}
#elif LV_USE_SDL
static void lv_linux_disp_init(void)
{
    lv_sdl_window_create(800, 480);

    /* Create input devices for SDL */
    lv_sdl_mouse_create();
    lv_sdl_keyboard_create();
}
#elif LV_USE_WAYLAND
    /* see backend/wayland.c */
#else
#endif



namespace GUI {
    LVGL::LVGL() {
        // Constructor implementation (if needed)
    }

    void LVGL::start() {
        pid_t c_pid = fork();
        uint32_t idle_time;
        
        if (c_pid == 0) {
            // Child process: Start LVGL GUI event loop
            std::cout << "[LVGL] Child process started" << std::endl;
            
            lv_init();
            std::cout << "[LVGL] LVGL initialized" << std::endl;
            
            lv_linux_disp_init();
            std::cout << "[LVGL] Display initialized" << std::endl;

            // Ensure root screen is always dark (prevents white gaps on carousel transitions)
            lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x0a1428), 0);
            lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);
            // Prevent the screen from auto-scrolling when side-set containers extend
            // beyond the 800px viewport (container at x=210 is 800px wide → extends to 1010).
            // Without this flag the LVGL viewport shifts right and hides the center set.
            lv_obj_clear_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);

            IPC::SharedMemory ipc_from_audio("TinyFirmwareSharedMemory_AudioToGUI", "gui");
            IPC::SharedMemory ipc_to_audio("TinyFirmwareSharedMemory_GUIToAudio", "gui");
            
            ipc_from_audio.startReaderThread();
            
            std::cout << "[LVGL] Connected to audio engine" << std::endl;
            
            // Initialize polyrhythmic carousel UI
            std::cout << "[LVGL] Creating PolyRhythmCarousel..." << std::endl;
            GUI::PolyRhythmCarousel carousel;
            carousel.init(lv_screen_active());
            std::cout << "[LVGL] PolyRhythmCarousel initialized!" << std::endl;
            
            // Wire IPC callbacks so user interactions are sent to the audio engine
            carousel.setOnPointToggle([&ipc_to_audio](size_t ringIdx, size_t pointIdx) {
                std::string msg = "POLY_TOGGLE:" + std::to_string(ringIdx) + "|" +
                                  std::to_string(pointIdx);
                ipc_to_audio.send(msg);
                std::cout << "[LVGL] Sent: " << msg << std::endl;
            });
            
            carousel.setOnShapeAdd([&ipc_to_audio](size_t ringIdx, uint32_t subdivision, double offset) {
                char buf[96];
                snprintf(buf, sizeof(buf), "POLY_ADD_SHAPE:%zu|%u|%.6f", ringIdx, subdivision, offset);
                ipc_to_audio.send(std::string(buf));
                std::cout << "[LVGL] Sent: " << buf << std::endl;
            });
            
            carousel.setOnShapeRemove([&ipc_to_audio](size_t ringIdx, size_t shapeIdx) {
                std::string msg = "POLY_REMOVE_SHAPE:" + std::to_string(ringIdx) + "|" +
                                  std::to_string(shapeIdx);
                ipc_to_audio.send(msg);
                std::cout << "[LVGL] Sent: " << msg << std::endl;
            });

            carousel.setOnShapeModify([&ipc_to_audio](size_t ringIdx, size_t shapeIdx,
                                                     uint32_t newSubdiv, double newOffset) {
                char buf[128];
                snprintf(buf, sizeof(buf), "POLY_MOD_SHAPE:%zu|%zu|%u|%.6f",
                         ringIdx, shapeIdx, newSubdiv, newOffset);
                ipc_to_audio.send(std::string(buf));
                std::cout << "[LVGL] Sent: " << buf << std::endl;
            });

            carousel.setOnRotate([&ipc_to_audio](size_t ringIdx, int amount) {
                std::string msg = "POLY_ROTATE:" + std::to_string(ringIdx) + "|" +
                                  std::to_string(amount);
                ipc_to_audio.send(msg);
                std::cout << "[LVGL] Sent: " << msg << std::endl;
            });

            carousel.setOnRingSelect([&ipc_to_audio](size_t ringIdx) {
                std::string msg = "POLY_SET_SELECTED_RING:" + std::to_string(ringIdx);
                ipc_to_audio.send(msg);
                std::cout << "[LVGL] Sent: " << msg << std::endl;
            });

            carousel.setOnModeChange([&ipc_to_audio](const std::string& mode) {
                std::string msg = "POLY_HW_MODE:" + mode;
                ipc_to_audio.send(msg);
                std::cout << "[LVGL] Sent: " << msg << std::endl;
            });
            
            // Request initial data from audio engine
            ipc_to_audio.send("GET_INITIAL_SETUP");

            bool is_playing = false;

#if LV_USE_SDL
            // SDL-only keyboard bridge used for desktop testing.
            lv_group_t* group = lv_group_create();
            lv_group_add_obj(group, lv_screen_active());
            lv_indev_t* keyboard_indev = lv_sdl_keyboard_create();
            lv_indev_set_group(keyboard_indev, group);

            std::cout << "[LVGL] Keyboard input configured" << std::endl;

            auto key_event_cb = [](lv_event_t* e) {
                uint32_t key = lv_event_get_key(e);
                void** user_data = static_cast<void**>(lv_event_get_user_data(e));
                bool* is_playing_ptr = static_cast<bool*>(user_data[0]);
                IPC::SharedMemory* ipc = static_cast<IPC::SharedMemory*>(user_data[1]);
                GUI::PolyRhythmCarousel* ui  = static_cast<GUI::PolyRhythmCarousel*>(user_data[2]);

                std::cout << "[LVGL] Key pressed: " << key << std::endl;

                if (key == LV_KEY_LEFT) {
                    ui->navigateLeft();
                } else if (key == LV_KEY_RIGHT) {
                    ui->navigateRight();
                } else if (key == LV_KEY_ENTER || key == ' ' || key == 32) {
                    *is_playing_ptr = !(*is_playing_ptr);
                    if (*is_playing_ptr) {
                        ipc->send("PLAY");
                        ui->setPlaying(true);
                        std::cout << "[LVGL] PLAY" << std::endl;
                    } else {
                        ipc->send("PAUSE");
                        ui->setPlaying(false);
                        std::cout << "[LVGL] PAUSE" << std::endl;
                    }
                }
            };

            static void* event_user_data[3];
            event_user_data[0] = &is_playing;
            event_user_data[1] = &ipc_to_audio;
            event_user_data[2] = &carousel;
            lv_obj_add_event_cb(lv_screen_active(), key_event_cb, LV_EVENT_KEY, event_user_data);

            std::cout << "[LVGL] Event callback registered" << std::endl;
#else
            (void)is_playing;
#endif
            
            while (true) {
                // ── Process IPC messages from audio engine ───────────────────
                auto& queue = ipc_from_audio.getMessageQueue();
                while (auto msg = queue.tryPop()) {
                    std::string message = msg.value();
                    if (message.rfind("POLY_PLAYHEAD:", 0) != 0) {
                        std::cout << "[LVGL] Received: " << message << std::endl;
                    }
                    
                    if (message.rfind("PLAY_STATE:", 0) == 0) {
                        const std::string state = message.substr(11);
                        const bool playing = (state == "1" || state == "true" || state == "on");
                        is_playing = playing;
                        carousel.setPlaying(playing);

                    } else if (message.substr(0, 4) == "BPM:") {
                        int bpm = std::stoi(message.substr(4));
                        carousel.setBPM(bpm);
                        
                    } else if (message == "POLY_REBUILD") {
                        // Engine signals that shapes changed — clear all ring data
                        carousel.clearAllRings();
                        
                    } else if (message.substr(0, 10) == "POLY_RING:") {
                        // POLY_RING:idx|name|color_hex|radius
                        std::string data = message.substr(10);
                        size_t p1 = data.find('|');
                        size_t p2 = data.find('|', p1 + 1);
                        size_t p3 = data.find('|', p2 + 1);
                        if (p1 != std::string::npos && p2 != std::string::npos && p3 != std::string::npos) {
                            // idx is implicit (order of addRing calls)
                            std::string name = data.substr(p1 + 1, p2 - p1 - 1);
                            std::string colorHex = data.substr(p2 + 1, p3 - p2 - 1);
                            uint32_t colorVal = static_cast<uint32_t>(std::stoul(colorHex, nullptr, 16));
                            lv_color_t color = lv_color_hex(colorVal);
                            int radius = std::stoi(data.substr(p3 + 1));
                            carousel.addRing(name, color, radius);
                            std::cout << "[LVGL] Ring added: " << name << " r=" << radius << std::endl;
                        }
                        
                    } else if (message.substr(0, 16) == "POLY_RING_SHAPE:") {
                        // POLY_RING_SHAPE:ringIdx|name|subdivision|offset|euclideanHits
                        std::string data = message.substr(16);
                        size_t p1 = data.find('|');
                        size_t p2 = data.find('|', p1 + 1);
                        size_t p3 = data.find('|', p2 + 1);
                        size_t p4 = data.find('|', p3 + 1);
                        if (p1 != std::string::npos && p2 != std::string::npos &&
                            p3 != std::string::npos && p4 != std::string::npos) {
                            size_t ringIdx = std::stoull(data.substr(0, p1));
                            ShapeInfo info;
                            info.name = data.substr(p1 + 1, p2 - p1 - 1);
                            info.subdivision = static_cast<uint32_t>(std::stoul(data.substr(p2 + 1, p3 - p2 - 1)));
                            info.offset = std::stod(data.substr(p3 + 1, p4 - p3 - 1));
                            info.euclideanHits = static_cast<uint32_t>(std::stoul(data.substr(p4 + 1)));
                            carousel.addRingShape(ringIdx, info);
                            std::cout << "[LVGL] Ring " << ringIdx << " shape: " << info.name
                                      << " (n=" << info.subdivision << ")" << std::endl;
                        }
                        
                    } else if (message.substr(0, 19) == "POLY_RING_TIMELINE:") {
                        // POLY_RING_TIMELINE:ringIdx|p0,p1,...
                        std::string data = message.substr(19);
                        size_t sep = data.find('|');
                        if (sep != std::string::npos) {
                            size_t ringIdx = std::stoull(data.substr(0, sep));
                            std::string phasesStr = data.substr(sep + 1);
                            std::vector<double> phases;
                            std::istringstream ss(phasesStr);
                            std::string token;
                            while (std::getline(ss, token, ',')) {
                                if (!token.empty()) {
                                    phases.push_back(std::stod(token));
                                }
                            }
                            carousel.setRingTimeline(ringIdx, phases);
                            std::cout << "[LVGL] Ring " << ringIdx << " timeline: "
                                      << phases.size() << " points" << std::endl;
                        }
                        
                    } else if (message.substr(0, 17) == "POLY_RING_ACTIVE:") {
                        // POLY_RING_ACTIVE:ringIdx|0,1,0,...
                        std::string data = message.substr(17);
                        size_t sep = data.find('|');
                        if (sep != std::string::npos) {
                            size_t ringIdx = std::stoull(data.substr(0, sep));
                            std::string pointsStr = data.substr(sep + 1);
                            std::vector<bool> active;
                            std::istringstream pss(pointsStr);
                            std::string ptoken;
                            while (std::getline(pss, ptoken, ',')) {
                                active.push_back(ptoken == "1");
                            }
                            carousel.setRingActivePoints(ringIdx, active);
                            std::cout << "[LVGL] Ring " << ringIdx << " active: "
                                      << active.size() << " points" << std::endl;
                        }

                    } else if (message.substr(0, 12) == "POLY_TOGGLE:") {
                        // POLY_TOGGLE:ringIdx|pointIdx
                        std::string data = message.substr(12);
                        size_t sep = data.find('|');
                        if (sep != std::string::npos) {
                            size_t ringIdx = std::stoull(data.substr(0, sep));
                            size_t pointIdx = std::stoull(data.substr(sep + 1));
                            carousel.togglePointLocal(ringIdx, pointIdx);
                            carousel.setSelectedPoint(ringIdx, pointIdx);
                        }

                    } else if (message.substr(0, 20) == "POLY_SELECTED_POINT:") {
                        // POLY_SELECTED_POINT:ringIdx|pointIdx
                        std::string data = message.substr(20);
                        size_t sep = data.find('|');
                        if (sep != std::string::npos) {
                            size_t ringIdx = std::stoull(data.substr(0, sep));
                            size_t pointIdx = std::stoull(data.substr(sep + 1));
                            carousel.setSelectedPoint(ringIdx, pointIdx);
                        }

                    } else if (message.substr(0, 13) == "POLY_HW_MODE:") {
                        carousel.setHardwareMode(message.substr(13));

                    } else if (message == "POLY_NAV_LEFT") {
                        carousel.navigateLeft();

                    } else if (message == "POLY_NAV_RIGHT") {
                        carousel.navigateRight();
                        
                    } else if (message.substr(0, 12) == "POLY_SELECT:") {
                        // POLY_SELECT:ringIdx — restore selected ring after rebuild
                        std::string data = message.substr(12);
                        size_t ringIdx = std::stoull(data);
                        carousel.selectRing(ringIdx);
                        std::cout << "[LVGL] Ring " << ringIdx << " selected" << std::endl;
                        
                    } else if (message == "POLY_READY") {
                        // All ring data received — rebuild visuals
                        carousel.rebuildFromData();
                        std::cout << "[LVGL] Poly UI rebuilt from data" << std::endl;
                        
                    } else if (message.substr(0, 14) == "POLY_PLAYHEAD:") {
                        // POLY_PLAYHEAD:phase|bpm
                        std::string data = message.substr(14);
                        size_t sep = data.find('|');
                        if (sep != std::string::npos) {
                            if (!is_playing) {
                                is_playing = true;
                                carousel.setPlaying(true);
                            }
                            double phase = std::stod(data.substr(0, sep));
                            int bpm = std::stoi(data.substr(sep + 1));
                            carousel.setPlayheadPhase(phase);
                            carousel.setBPM(bpm);
                        }
                    }
                }
                // Update UI animation
                carousel.update();
                // Handle LVGL
                idle_time = lv_timer_handler();
                usleep(idle_time * 1000);
            }
        } else if (c_pid > 0) {
            // Parent process: Continue immediately, audio engine will run in parent
            std::cout << "[Main] GUI started in child process (PID: " << c_pid << ")" << std::endl;
        } else {
            // Fork failed
            perror("Fork failed");
        }
    }

    void LVGL::stop() {
        // Stop child loop process
    }

    void LVGL::process() {
        // Process GUI events (empty for now as LVGL runs in separate process)
    }

    void LVGL::handleWaveformEvent(const double* samples, size_t numSamples) {
        // Handle waveform data for visualization
        // TODO: Implement waveform visualization
    }

    void LVGL::handleModuleListEvent(const std::map<std::string, Common::ModuleInfo>& modules) {
        // Handle module list updates
        // TODO: Update module list display
    }

    void LVGL::handleConnectionListEvent(const std::vector<std::shared_ptr<Engine::ConnectionInfo>>& connections,
                                        const std::vector<std::string>& modules) {
        // Handle connection updates
        // TODO: Update connection display
    }

    void LVGL::handleMessageEvent(Common::logType type, const std::string& message) {
        // Handle log messages
        // TODO: Display messages in GUI
    }
}
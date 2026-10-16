#include "maze.hpp"

#ifndef NDEBUG
#include "capture/menucapturelayer.hpp"
#endif
#include "core/file.hpp"
#include "core/settings.hpp"
#include "core/window.hpp"
#include "entrypoint.hpp"
#include "event/applicationevent.hpp"
#include "event/event.hpp"
#include "layer/audiolayer.hpp"
#include "layer/confirmlayer.hpp"
#include "layer/exitlayer.hpp"
#include "layer/introlayer.hpp"
#include "layer/keymaplayer.hpp"
#include "layer/optionlayer.hpp"
#include "layer/scrollmenulayer.hpp"
#include "ui/confirmlayout.hpp"
#include "ui/fontmetrics.hpp"
#include "ui/keyhints.hpp"
#include "ui/menufontsize.hpp"
#include "ui/menulayout.hpp"
#include "ui/yogadump.hpp"
#include "version.hpp"

#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace game {
using sponge::platform::glfw::core::ApplicationSpecification;

#ifdef NDEBUG
Maze::Maze(ApplicationSpecification specification) :
    Application(std::move(specification)) {
    // Base class handles singleton pattern
}
#else
Maze::Maze(ApplicationSpecification specification, CaptureOptions capture) :
    Application(std::move(specification)), capture(std::move(capture)) {
    // Base class handles singleton pattern
}
#endif

bool Maze::onUserCreate() {
    pushOverlay(imguiLayer);
    imguiLayer->setActive(false);

    pushOverlay(splashScreenLayer);
    pushOverlay(exitLayer);
    pushOverlay(optionLayer);
    pushOverlay(keyMapLayer);
    pushOverlay(audioLayer);
    pushOverlay(confirmLayer);

    pushLayer(mazeLayer);
    pushLayer(loadingLayer);
    pushLayer(introLayer);

    exitLayer->setActive(false);
    introLayer->setActive(false);
    loadingLayer->setActive(false);
    mazeLayer->setActive(false);
    keyMapLayer->setActive(false);
    optionLayer->setActive(false);
    audioLayer->setActive(false);
    confirmLayer->setActive(false);

#ifndef NDEBUG
    setUpCapture();
#endif

    const auto savedAa = sponge::core::Settings::getUInt32(
        "video.aa", static_cast<uint32_t>(thread::AntiAliasing::Taa));
    setAntiAliasing(savedAa <
                            static_cast<uint32_t>(thread::AntiAliasing::Count) ?
                        static_cast<thread::AntiAliasing>(savedAa) :
                        thread::AntiAliasing::Taa);
    setBloomEnabled(
        sponge::core::Settings::getBool("video.bloomEnabled", true));

    using sponge::platform::opengl::scene::ToneMapper;
    const auto savedTone = sponge::core::Settings::getUInt32(
        "video.toneMapper", static_cast<uint32_t>(ToneMapper::Agx));
    mazeLayer->setToneMapper(savedTone <
                                     static_cast<uint32_t>(ToneMapper::Count) ?
                                 static_cast<ToneMapper>(savedTone) :
                                 ToneMapper::Agx);

    // After the settings above, so the maze is configured when it loads.
#ifndef NDEBUG
    startCaptureLoad();
#endif

    return true;
}

#ifndef NDEBUG
void Maze::setUpCapture() {
    // A layer starts active, so the splash has to be turned off explicitly. Its
    // timeout is also what starts the intro.
    splashScreenLayer->setActive(capture.frames == 0 && capture.menu.empty());
    if (!capture.menu.empty()) {
        // Last in the stack, so it reads the frame after every menu has drawn.
        pushOverlay(
            std::make_shared<game::capture::MenuCaptureLayer>(capture.menu));
        if (capture.menu == "display") {
            ui::showOptionTab(ui::OptionTab::Display);
        } else if (capture.menu == "keyboard") {
            ui::showOptionTab(ui::OptionTab::Keyboard);
        } else if (capture.menu == "audio") {
            ui::showOptionTab(ui::OptionTab::Audio);
        } else if (capture.menu == "confirm") {
            ui::showOptionTab(ui::OptionTab::Display);
            confirmLayer->show(layer::displayChangeTitle,
                               layer::displayChangeMessage, nullptr, 15.F);
        } else if (capture.menu == "reset") {
            ui::showOptionTab(ui::OptionTab::Keyboard);
            confirmLayer->show(layer::resetBindingsTitle,
                               layer::resetBindingsMessage, nullptr, 0.F,
                               false);
        } else if (capture.menu == "exit") {
            exitLayer->setActive(true);
        } else {
            introLayer->setActive(true);
        }
    }
    if (capture.frames > 0) {
        mazeLayer->enableCapture(capture.frames, capture.nanStats, capture.off,
                                 capture.walk, capture.cascades);
    }
}

void Maze::startCaptureLoad() {
    if (capture.frames > 0) {
        loadingLayer->setActive(true);
    }
}
#endif

bool Maze::onUserUpdate(const double elapsedTime) {
    if (!isRunning) {
        return false;
    }

    return Application::onUserUpdate(elapsedTime);
}

bool Maze::onUserDestroy() {
    return true;
}

void Maze::onEvent(sponge::event::Event& event) {
    sponge::event::EventDispatcher dispatcher(event);

    dispatcher.dispatch<sponge::event::WindowCloseEvent>(
        [this](const sponge::event::WindowCloseEvent& ev) {
            return this->onWindowClose(ev);
        });

    Application::onEvent(event);
}

bool Maze::onWindowClose(const sponge::event::WindowCloseEvent&) {
    isRunning = false;
    return true;
}
}  // namespace game

std::unique_ptr<sponge::platform::glfw::core::Application>
    sponge::platform::glfw::core::createApplication(
        [[maybe_unused]] const int argc, [[maybe_unused]] char** argv) {
    using sponge::core::Settings;

    bool     capturing     = false;
    bool     decorated     = true;
    uint32_t captureWidth  = 1600;
    uint32_t captureHeight = 900;
#ifndef NDEBUG
    game::CaptureOptions capture;

    // Debug builds only; a release build ignores these flags.
    // --capture-frames N or --capture-frames=N: skip the menus, render N
    // frames, write the last one as float maps and exit. --dump-nan-stats:
    // also scan every frame (N defaults to 600). A missing or non-numeric N
    // leaves the normal start. --capture-off=a,b: turn passes off for the run
    // (see MazeLayer::enableCapture). --capture-walk=D: walk D world units
    // per frame along +X instead of turning. --capture-cascades=N: use N
    // shadow cascades. --capture-menu=display, keyboard, audio or exit: show
    // that menu screen (any other name shows the intro), write
    // capture_menu_<name>.png and exit. --capture-size=WxH: the window size
    // of any capture run. --dump-yoga=intro, exit, display, keyboard, audio,
    // confirm or reset: print that menu's layout as yogalayout.dev playground
    // JSX.
    constexpr std::string_view          captureFlag = "--capture-frames";
    std::string_view                    dumpYoga;
    const std::vector<std::string_view> args{ argv + 1, argv + argc };
    const auto parseCount = [&capture](const std::string_view value) {
        std::from_chars(value.data(), value.data() + value.size(),
                        capture.frames);
    };
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == captureFlag && i + 1 < args.size()) {
            parseCount(args[i + 1]);
        } else if (args[i].starts_with(captureFlag) &&
                   args[i].size() > captureFlag.size() &&
                   args[i][captureFlag.size()] == '=') {
            parseCount(args[i].substr(captureFlag.size() + 1));
        } else if (args[i] == "--dump-nan-stats") {
            capture.nanStats = true;
        } else if (args[i].starts_with("--capture-off=")) {
            capture.off = std::string(args[i].substr(14));
        } else if (args[i].starts_with("--capture-walk=")) {
            const auto value = args[i].substr(15);
            std::from_chars(value.data(), value.data() + value.size(),
                            capture.walk);
        } else if (args[i].starts_with("--capture-menu=")) {
            capture.menu = std::string(args[i].substr(15));
        } else if (args[i].starts_with("--dump-yoga=")) {
            dumpYoga = args[i].substr(12);
        } else if (args[i].starts_with("--capture-size=")) {
            const auto value = args[i].substr(15);
            const auto x     = value.find('x');
            if (x != std::string_view::npos) {
                std::from_chars(value.data(), value.data() + x, captureWidth);
                std::from_chars(value.data() + x + 1,
                                value.data() + value.size(), captureHeight);
            }
        } else if (args[i].starts_with("--capture-cascades=")) {
            const auto value = args[i].substr(19);
            std::from_chars(value.data(), value.data() + value.size(),
                            capture.cascades);
        }
    }
    if (!dumpYoga.empty()) {
        // --dump-yoga=intro, exit, display, keyboard, audio, confirm or reset:
        // print the menu's Yoga tree as playground JSX at the --capture-size
        // window size and exit.
        const auto width  = static_cast<float>(captureWidth);
        const auto height = static_cast<float>(captureHeight);

        if (dumpYoga == "confirm" || dumpYoga == "reset") {
            const bool            timed = dumpYoga == "confirm";
            game::ui::FontMetrics metrics(sponge::core::File::getResourceDir() +
                                          "/fonts/inter.ktx2");
            const auto            measure = metrics.measure();
            const auto            fontSize =
                game::ui::menuFontSizeForWidth(static_cast<uint32_t>(width));
            const auto title   = timed ? game::layer::displayChangeTitle :
                                         game::layer::resetBindingsTitle;
            const auto message = timed ? game::layer::displayChangeMessage :
                                         game::layer::resetBindingsMessage;
            game::ui::ConfirmLayout layout;
            layout.calculate(
                width, height, fontSize, measure(title, fontSize),
                measure(message, fontSize),
                timed ? std::optional{ measure("Reverting in 15", fontSize) } :
                        std::nullopt);
            std::fputs(game::ui::yogaToPlayground(layout.root()).c_str(),
                       stdout);
            std::exit(0);
        }

        // The option tabs share one layout, with the rows placed by MenuScroll.
        using game::layer::AudioMenuItem;
        using game::layer::KeyMapItem;
        using game::layer::OptionMenuItem;
        const auto tabRows =
            dumpYoga == "keyboard" ? static_cast<size_t>(KeyMapItem::Count) :
            dumpYoga == "audio"    ? static_cast<size_t>(AudioMenuItem::Count) :
            dumpYoga == "display" ? static_cast<size_t>(OptionMenuItem::Count) :
                                    0;
        const bool isExit = dumpYoga == "exit";
        const auto rows =
            isExit ? static_cast<size_t>(game::layer::ExitMenuItem::Count) :
                     static_cast<size_t>(game::layer::IntroMenuItem::Count);
        const std::span<const game::ui::KeyHint> hints =
            dumpYoga == "keyboard" ?
                std::span<const game::ui::KeyHint>{
                    game::layer::keyMapKeyHints
                } :
            tabRows > 0 ?
                std::span<const game::ui::KeyHint>{
                    game::layer::adjustKeyHints
                } :
            isExit ?
                std::span<const game::ui::KeyHint>{
                    game::layer::exitKeyHints
                } :
                std::span<const game::ui::KeyHint>{
                    game::layer::introKeyHints
                };
        game::ui::MenuLayout layout(tabRows > 0 ? tabRows : rows,
                                    tabRows > 0 ?
                                        game::layer::scrollMenuLayout :
                                    isExit ? game::layer::exitMenuLayout :
                                             game::layer::introMenuLayout,
                                    hints);

        game::ui::FontMetrics metrics(sponge::core::File::getResourceDir() +
                                      "/fonts/inter.ktx2");
        const auto            measure = metrics.measure();
        if (tabRows > 0) {
            layout.setTabLabelSizes(game::ui::tabLabelSizes(width, measure));
        }
        layout.setKeyHintSizes(
            game::ui::keyHintSizes(hints, width, false, measure), false);
        layout.calculate(width, height);
        std::fputs(game::ui::yogaToPlayground(layout.root()).c_str(), stdout);
        std::exit(0);
    }
    if (capture.nanStats && capture.frames == 0) {
        capture.frames = 600;
    }
    capturing = capture.frames > 0 || !capture.menu.empty();
    decorated = capture.menu.empty();
#endif

    // Files from before the three display modes hold only video.fullscreen.
    constexpr auto modeCount =
        static_cast<uint32_t>(sponge::core::DisplayMode::Count);
    auto savedMode = Settings::getUInt32("video.displayMode", modeCount);
    if (savedMode >= modeCount) {
        savedMode =
            static_cast<uint32_t>(Settings::getBool("video.fullscreen", true) ?
                                      sponge::core::DisplayMode::FullScreen :
                                      sponge::core::DisplayMode::Windowed);
    }

    // A capture run uses a fixed window and no vsync, so runs compare.
    const auto spec = ApplicationSpecification{
        .name = game::project_name,
        .width =
            capturing ? captureWidth : Settings::getUInt32("video.width", 0),
        .height =
            capturing ? captureHeight : Settings::getUInt32("video.height", 0),
        .displayMode = capturing ?
                           sponge::core::DisplayMode::Windowed :
                           static_cast<sponge::core::DisplayMode>(savedMode),
        .vsync       = !capturing && Settings::getBool("video.vsync", true),
        // An undecorated window can be larger than the screen.
        .decorated            = decorated,
        .throttleInBackground = !capturing,
    };

#ifdef NDEBUG
    return std::make_unique<game::Maze>(spec);
#else
    return std::make_unique<game::Maze>(spec, std::move(capture));
#endif
}

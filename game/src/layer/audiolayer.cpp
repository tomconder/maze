#include "layer/audiolayer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>

#include <yoga/Yoga.h>

#include "core/base.hpp"
#include "core/settings.hpp"
#include "event/event.hpp"
#include "input/gameaction.hpp"
#include "input/inputcontext.hpp"
#include "input/mousecode.hpp"
#include "platform/audio/audio.hpp"
#include "platform/glfw/core/application.hpp"
#include "platform/glfw/core/inputmanager.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/scene/bitmapfont.hpp"
#include "platform/opengl/scene/quad.hpp"
#include "resourcemanager.hpp"
#include "scene/orthocamera.hpp"
#include "ui/button.hpp"
#include "ui/keyhints.hpp"
#include "ui/menufontsize.hpp"
#include "ui/menulayout.hpp"
#include "ui/menuselection.hpp"
#include "ui/slider.hpp"
#include "ui/tabbar.hpp"

namespace {
constexpr std::string_view cameraName = "intro";
constexpr std::string_view fontName   = "inter";
constexpr std::string_view fontPath   = "/fonts/inter.ktx2";

constexpr std::string_view returnMessage = "Return";

constexpr glm::vec4 backgroundColor    = { 0.F, 0.F, 0.F, 1.F };
constexpr glm::vec4 buttonColor        = { 0.F, 0.F, 0.F, 0.F };
constexpr glm::vec4 hoverColor         = { 0.84F, 0.84F, 0.84F, 0.14F };
constexpr glm::vec3 textColor          = { 1.F, 1.F, 1.F };
constexpr glm::vec3 arrowDisabledColor = { 0.4F, 0.4F, 0.4F };
constexpr glm::vec4 textHoverColor     = { 0.84F, 0.04F, 0.04F, 0.14F };

uint32_t        fontSize            = 48;
constexpr float textMarginLeft      = 26.F;
constexpr float cornerRadius        = 12.F;
constexpr float selectedBorderWidth = 3.F;
constexpr float volumeStep          = 0.05F;

using game::layer::AudioMenuItem;

constexpr size_t rowCount = static_cast<size_t>(AudioMenuItem::Count);

struct RowDef {
    AudioMenuItem    item;
    std::string_view label;
};

// Every row except Return, which is a Button rather than a slider.
constexpr std::array volumeRows = {
    RowDef{ .item = AudioMenuItem::MasterVolume, .label = "Master Volume" },
    RowDef{ .item = AudioMenuItem::SfxVolume, .label = "Sound Effects" },
    RowDef{ .item = AudioMenuItem::MusicVolume, .label = "Music" },
};

// rowNodes are created by index, so the table must stay in enum order and
// cover every row but Return.
static_assert(volumeRows.size() + 1 == rowCount);
static_assert([] {
    for (size_t i = 0; i < volumeRows.size(); i++) {
        if (static_cast<size_t>(volumeRows[i].item) != i) {
            return false;
        }
    }
    return true;
}());

// Rows are laid out top-to-bottom in AudioMenuItem order, so the enum doubles
// as the row index.
std::array<YGNodeRef, rowCount> rowNodes{};

YGNodeRef menuBackgroundNode = nullptr;
YGNodeRef menuNode           = nullptr;
YGNodeRef rootNode           = nullptr;

constexpr std::array<game::ui::KeyHint, 4> audioKeyHints = {
    game::ui::KeyHint{ "keyboard_arrows_vertical", "xbox_dpad_vertical",
                       "Navigate" },
    game::ui::KeyHint{ "keyboard_arrows_horizontal", "xbox_dpad_horizontal",
                       "Change" },
    game::ui::KeyHint{ "keyboard_enter", "xbox_button_a", "Select" },
    game::ui::KeyHint{ "keyboard_escape", "xbox_button_b", "Back" },
};

std::unique_ptr<game::ui::Button> returnButton;

std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> menuFont;
std::unique_ptr<sponge::platform::opengl::scene::Quad>       quad;
std::shared_ptr<game::scene::OrthoCamera>                    orthoCamera;

sponge::platform::glfw::core::InputManager& inputManager() {
    return sponge::platform::glfw::core::Application::get().getInputManager();
}

bool contains(const float x, const float y, const float w, const float h,
              const float px, const float py) {
    return px >= x && px <= x + w && py >= y && py <= y + h;
}
}  // namespace

namespace game::layer {
using sponge::event::Event;
using sponge::event::EventDispatcher;
using sponge::event::MouseButtonPressedEvent;
using sponge::event::MouseButtonReleasedEvent;
using sponge::event::MouseMovedEvent;
using sponge::event::WindowResizeEvent;
using sponge::platform::opengl::renderer::AssetManager;
using sponge::platform::opengl::scene::FontCreateInfo;
using sponge::platform::opengl::scene::Quad;

AudioLayer::AudioLayer() : Layer("audio") {}

void AudioLayer::onAttach() {
    const auto fontCreateInfo = FontCreateInfo{
        .name = std::string(fontName),
        .path = std::string(fontPath),
    };
    menuFont = AssetManager::createFont(fontCreateInfo);

    const auto orthoCameraCreateInfo =
        scene::OrthoCameraCreateInfo{ .name = std::string(cameraName) };
    orthoCamera = ResourceManager::createOrthoCamera(orthoCameraCreateInfo);

    quad = std::make_unique<Quad>();

    fontSize = ui::menuFontSizeForWidth(orthoCamera->getWidth());

    returnButton = ui::makeMenuButton(returnMessage, fontSize, menuFont,
                                      buttonColor, textColor);

    const ui::SliderCreateInfo sliderCreateInfo{
        .font               = menuFont,
        .fontSize           = fontSize,
        .textColor          = textColor,
        .arrowDisabledColor = arrowDisabledColor,
        .textMarginLeft     = textMarginLeft,
    };
    masterVolumeSlider = std::make_unique<ui::Slider>(sliderCreateInfo);
    sfxVolumeSlider    = std::make_unique<ui::Slider>(sliderCreateInfo);
    musicVolumeSlider  = std::make_unique<ui::Slider>(sliderCreateInfo);

    using sponge::core::Settings;
    masterVolumeSlider->setValue(
        static_cast<float>(Settings::getUInt32("audio.masterVolume", 100)) /
        100.F);
    sfxVolumeSlider->setValue(
        static_cast<float>(Settings::getUInt32("audio.sfxVolume", 100)) /
        100.F);
    musicVolumeSlider->setValue(
        static_cast<float>(Settings::getUInt32("audio.musicVolume", 100)) /
        100.F);

    for (const auto& shader : { menuFont->getShader(), Quad::getShader() }) {
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }

    // no spacer above: the rows start under the tab bar
    const auto skeleton = ui::buildMenuSkeleton(45.F, 0.F);
    rootNode            = skeleton.root;
    menuNode            = skeleton.menu;
    menuBackgroundNode  = skeleton.menuBackground;

    for (size_t i = 0; i < rowNodes.size(); i++) {
        rowNodes[i] = ui::makeMenuRow(menuBackgroundNode, static_cast<int>(i));
    }

    const auto width  = static_cast<float>(orthoCamera->getWidth());
    const auto height = static_cast<float>(orthoCamera->getHeight());
    recalculateLayout(width, height);
}

void AudioLayer::onDetach() {
    YGNodeFreeRecursive(rootNode);
}

void AudioLayer::onEvent(Event& event) {
    EventDispatcher dispatcher(event);

    dispatcher.dispatch<MouseButtonPressedEvent>(
        [this](const MouseButtonPressedEvent& mouseEvent) {
            return isActive() ? onMouseButtonPressed(mouseEvent) : false;
        });
    dispatcher.dispatch<MouseButtonReleasedEvent>(
        [this](const MouseButtonReleasedEvent& mouseEvent) {
            return isActive() ? onMouseButtonReleased(mouseEvent) : false;
        });
    dispatcher.dispatch<MouseMovedEvent>(
        [this](const MouseMovedEvent& mouseMovedEvent) {
            return isActive() ? onMouseMoved(mouseMovedEvent) : false;
        });
    dispatcher.dispatch<WindowResizeEvent>(
        [this](const WindowResizeEvent& windowResizeEvent) {
            return onWindowResize(windowResizeEvent);
        });
}

bool AudioLayer::onUpdate(const double elapsedTime) {
    {
        using sponge::input::GameAction;
        auto& mgr = inputManager();
        mgr.setActiveContext(sponge::input::InputContext::Menu);

        {
            const auto& input = mgr.getSnapshot();
            if (!wasActiveLastFrame) {
                waitForConfirmRelease = input.isHeld(GameAction::MenuConfirm);
            } else if (waitForConfirmRelease &&
                       !input.isHeld(GameAction::MenuConfirm)) {
                waitForConfirmRelease = false;
            }
        }

        if (wasActiveLastFrame) {
            const auto& input = mgr.getSnapshot();

            if (ui::stepSelection(input, selectedItem)) {
                ui::playHoverClick();
            }

            if (auto* const slider = sliderFor(selectedItem);
                slider != nullptr) {
                bool changed = false;
                if (input.isActive(GameAction::MenuLeft)) {
                    changed = slider->step(-volumeStep);
                }
                if (input.isActive(GameAction::MenuRight)) {
                    changed = slider->step(volumeStep) || changed;
                }
                if (changed) {
                    applyVolume(selectedItem, true);
                }
            }

            if (input.isActive(GameAction::TabNext)) {
                mgr.consumeActive(GameAction::TabNext);
                clearHoveredItems();
                ui::showOptionTab(ui::cycleTab(ui::OptionTab::Audio, 1));
            }
            if (input.isActive(GameAction::TabPrev)) {
                mgr.consumeActive(GameAction::TabPrev);
                clearHoveredItems();
                ui::showOptionTab(ui::cycleTab(ui::OptionTab::Audio, -1));
            }
            if (input.isActive(GameAction::MenuBack)) {
                mgr.consumeActive(GameAction::MenuBack);
                close();
            }
            if (!waitForConfirmRelease &&
                input.isActive(GameAction::MenuConfirm) &&
                selectedItem == AudioMenuItem::Return) {
                mgr.consumeActive(GameAction::MenuConfirm);
                close();
            }
        }
        wasActiveLastFrame = true;
    }

    for (const auto& shader : { menuFont->getShader(), Quad::getShader() }) {
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }

    const auto width  = static_cast<float>(orthoCamera->getWidth());
    const auto height = static_cast<float>(orthoCamera->getHeight());
    quad->render({ 0.F, 0.F }, { width, height }, backgroundColor);

    for (const auto& [item, label] : volumeRows) {
        const auto [x, y, w, h] = rowLayout(item);
        renderRowBackground(x, y, w, h, item);
        sliderFor(item)->onUpdate(x, y, w, h, label);
    }

    const auto [retX, retY, retW, retH] = rowLayout(AudioMenuItem::Return);
    returnButton->setPosition({ retX, retY }, { retX + retW, retY + retH });
    ui::updateMenuButtonVisuals(returnButton.get(),
                                selectedItem == AudioMenuItem::Return,
                                textHoverColor);
    UNUSED(returnButton->onUpdate(elapsedTime));

    // the strip lines up with the rows, so it starts at the first row's edge
    const auto [tabX, tabY, tabW, tabH] =
        rowLayout(AudioMenuItem::MasterVolume);
    ui::renderTabBar(ui::OptionTab::Audio, menuFont,
                     orthoCamera->getProjection(), width, tabX);

    ui::renderKeyHints(audioKeyHints, menuFont, orthoCamera->getProjection(),
                       width, height);

    if (!isActive()) {
        wasActiveLastFrame    = false;
        waitForConfirmRelease = false;
        draggingItem          = std::nullopt;
    }

    return true;
}

void AudioLayer::renderRowBackground(const float x, const float y,
                                     const float w, const float h,
                                     const AudioMenuItem item) const {
    const bool isSelected = selectedItem == item;
    const bool isHovered  = hoveredItem == item;
    quad->render({ x, y }, { x + w, y + h },
                 isSelected ? textHoverColor :
                 isHovered  ? hoverColor :
                              buttonColor,
                 cornerRadius, isSelected ? selectedBorderWidth : 0.F,
                 glm::vec4{ 1.F });
}

std::tuple<float, float, float, float>
    AudioLayer::rowLayout(const AudioMenuItem item) {
    const auto [rootX, rootY, rootW, rootH] = ui::getNodeLayout(
        rootNode, 0.F,
        ui::tabBarHeight(static_cast<float>(orthoCamera->getWidth())));
    const auto [menuX, menuY, menuW, menuH] =
        ui::getNodeLayout(menuNode, rootX, rootY);
    const auto [bgX, bgY, bgW, bgH] =
        ui::getNodeLayout(menuBackgroundNode, menuX, menuY);
    return ui::getNodeLayout(rowNodes[static_cast<size_t>(item)], bgX, bgY);
}

ui::Slider* AudioLayer::sliderFor(const AudioMenuItem item) const {
    switch (item) {
        case AudioMenuItem::MasterVolume:
            return masterVolumeSlider.get();
        case AudioMenuItem::SfxVolume:
            return sfxVolumeSlider.get();
        case AudioMenuItem::MusicVolume:
            return musicVolumeSlider.get();
        default:
            return nullptr;
    }
}

void AudioLayer::applyVolume(const AudioMenuItem item, const bool save) const {
    auto* const slider = sliderFor(item);
    if (slider == nullptr) {
        return;
    }

    using sponge::core::Settings;
    using sponge::platform::audio::Audio;

    const auto value   = slider->getValue();
    const auto percent = static_cast<uint32_t>(std::lround(value * 100.F));

    switch (item) {
        case AudioMenuItem::MasterVolume:
            Audio::setMasterVolume(value);
            Settings::set("audio.masterVolume", percent);
            break;
        case AudioMenuItem::SfxVolume:
            Audio::setSfxVolume(value);
            Settings::set("audio.sfxVolume", percent);
            break;
        case AudioMenuItem::MusicVolume:
            Audio::setMusicVolume(value);
            Settings::set("audio.musicVolume", percent);
            break;
        default:
            return;
    }

    if (save) {
        Settings::save();
    }
}

void AudioLayer::close() {
    clearHoveredItems();
    selectedItem = AudioMenuItem::MasterVolume;
    setActive(false);
}

void AudioLayer::recalculateLayout(const float width, const float height) {
    // leave room for the tab bar along the top and the key hint bar along the
    // bottom
    const auto usableHeight =
        std::max(0.F, ui::heightWithoutKeyHints(width, height) -
                          ui::tabBarHeight(width));
    for (auto* const row : rowNodes) {
        ui::setMenuRowHeight(row, width);
    }
    ui::pinMenuRowToBottom(rowNodes[+AudioMenuItem::Return], width);
    YGNodeStyleSetWidth(rootNode, width);
    YGNodeStyleSetHeight(rootNode, usableHeight);
    YGNodeCalculateLayout(rootNode, width, usableHeight, YGDirectionLTR);
}

bool AudioLayer::onMouseButtonPressed(const MouseButtonPressedEvent& event) {
    if (event.getMouseButton() != sponge::input::MouseButton::Button0) {
        return false;
    }

    auto [mouseX, mouseY] =
        sponge::platform::glfw::core::Application::get().getMousePosition();

    const auto [tabX, tabY, tabW, tabH] =
        rowLayout(AudioMenuItem::MasterVolume);
    const auto clickedTab =
        ui::tabBarHitTest(menuFont, static_cast<float>(orthoCamera->getWidth()),
                          tabX, { mouseX, mouseY });
    if (clickedTab) {
        clearHoveredItems();
        ui::showOptionTab(*clickedTab);
        return true;
    }

    if (returnButton->isInside({ mouseX, mouseY })) {
        close();
        return true;
    }

    for (const auto& [item, label] : volumeRows) {
        const auto [x, y, w, h] = rowLayout(item);
        if (!contains(x, y, w, h, mouseX, mouseY)) {
            continue;
        }

        selectedItem = item;

        auto* const slider = sliderFor(item);
        if (slider->isInsideTrack(mouseX, x, w)) {
            slider->setValue(slider->valueAtX(mouseX, x, w));
            applyVolume(item, false);
            draggingItem = item;
        }
        return true;
    }

    return true;
}

bool AudioLayer::onMouseMoved(const MouseMovedEvent& event) {
    const auto pos = glm::vec2{ event.getX(), event.getY() };

    if (draggingItem) {
        const auto [x, y, w, h] = rowLayout(*draggingItem);
        auto* const slider      = sliderFor(*draggingItem);
        slider->setValue(slider->valueAtX(pos.x, x, w));
        applyVolume(*draggingItem, false);
        return true;
    }

    ui::updateButtonHover(returnButton.get(), pos);

    std::optional<AudioMenuItem> nextHover;
    for (const auto& [item, label] : volumeRows) {
        const auto [x, y, w, h] = rowLayout(item);
        if (contains(x, y, w, h, pos.x, pos.y)) {
            nextHover = item;
            break;
        }
    }
    if (nextHover && nextHover != hoveredItem) {
        ui::playHoverClick();
    }
    hoveredItem = nextHover;

    return true;
}

bool AudioLayer::onMouseButtonReleased(const MouseButtonReleasedEvent& event) {
    if (event.getMouseButton() != sponge::input::MouseButton::Button0 ||
        !draggingItem) {
        return false;
    }

    applyVolume(*draggingItem, true);
    draggingItem = std::nullopt;
    return true;
}

bool AudioLayer::onWindowResize(const WindowResizeEvent& event) {
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());

    const auto width  = static_cast<float>(event.getWidth());
    const auto height = static_cast<float>(event.getHeight());
    recalculateLayout(width, height);

    const auto newFontSize = ui::menuFontSizeForWidth(event.getWidth());
    if (newFontSize != fontSize) {
        fontSize = newFontSize;
        returnButton->setFontSize(fontSize);
        for (auto* const slider :
             { masterVolumeSlider.get(), sfxVolumeSlider.get(),
               musicVolumeSlider.get() }) {
            slider->setFontSize(fontSize);
        }
    }

    return false;
}

void AudioLayer::clearHoveredItems() {
    returnButton->setHover(false);
    hoveredItem = std::nullopt;
}
}  // namespace game::layer

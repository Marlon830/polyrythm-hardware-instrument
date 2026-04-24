/*
@file WidgetFactory.hpp
@brief Declaration of the WidgetFactory class for creating GUI widgets.
@ingroup gui_widget
Defines the WidgetFactory class which provides a mechanism to create GUI widgets
based on string identifiers. It supports registration of widget creators for
dynamic instantiation.
*/

#ifndef WIDGET_FACTORY_HPP
    #define WIDGET_FACTORY_HPP

    #include <memory>
    #include <functional>
    #include <unordered_map>
    #include "gui/TUI/widget/IWidget.hpp"
    #include "gui/TUI/TUI.hpp"
    #include <string>

namespace GUI {

    /// @brief Type definition for widget creator functions.
    using Creator = std::function<std::shared_ptr<IWidget>(std::string, TUI*)>;

    /// @brief Factory class for creating GUI widgets.
    class WidgetFactory final {
    public:
        /// @brief Get the singleton instance of the WidgetFactory.
        /// @return Reference to the WidgetFactory instance.
        static WidgetFactory& instance();

        /// @brief Create a widget based on its string identifier.
        /// @param name The string identifier of the widget.
        /// @param tui Pointer to the TUI instance.
        /// @return A shared pointer to the created widget.
        std::shared_ptr<IWidget> create(std::string_view name, TUI* tui) const;

        /// @brief Register a widget creator function with a string identifier.
        /// @param key The string identifier for the widget.
        /// @param fn The creator function for the widget.
        /// @return True if registration was successful, false if the key already exists.
        static bool registerModule(std::string key, Creator fn);

        /// @brief Unregister a widget creator function by its string identifier.
        /// @param key The string identifier of the widget to unregister.
        static void unregisterModule(std::string_view key);
        
        /// @brief Get the internal registry of widget creators.
        /// @return Reference to the unordered map of widget creators.
        static std::unordered_map<std::string, Creator>& registry();

        //helpers to create get the id

        /// @brief Parse the ID from a widget name string.
        /// @param name The widget name string.
        /// @return The parsed ID as an unsigned integer.
        static unsigned parse_id(const std::string& name);
        
        /// @brief Parse the base name from a widget name string.
        /// @param name The widget name string.
        /// @return The base name as a string.
        static std::string parse_name(const std::string& name);

    private:
        WidgetFactory() : _id_counter(0) {}
        int _id_counter;
    };

    struct AutoRegisterWidget final {
        AutoRegisterWidget(std::string key, Creator fn);
    };

} // namespace GUI
#endif // WIDGET_FACTORY_HPP
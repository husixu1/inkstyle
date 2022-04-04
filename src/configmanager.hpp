#ifndef CONFIGMANAGER_HPP
#define CONFIGMANAGER_HPP

#include <QColor>
#include <QHash>
#include <QIcon>
#include <QMap>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QVector>
#include <yaml-cpp/yaml.h>

class ConfigManager : public QObject {
    Q_OBJECT

public:
    explicit ConfigManager(const QString &file, QObject *parent = nullptr);

    QColor panelBgColor;
    QColor buttonBgColor;
    QColor guideColor;
    quint8 panelMaxLevels;
    quint32 panelRadius;

    /// @brief Presetted icon shapes
    enum class PresetIconStyle { circle, square, hexagon };

    class ButtonInfo {
    public:
        ButtonInfo(
            const QByteArray &styleSvg, const QByteArray &userIconSvg,
            const QMap<QString, QString> &styles);

        /// @brief Generate preset icon svg for representing showing on buttons
        /// @details This function should be called to generate an icon when
        /// the user does not provide a custom icon.
        /// @param size size of the icon
        /// @param shape @see #IconShape
        /// @return The icon svg stored in a byte array
        QByteArray genIconSvg(
            QSizeF size = QSizeF(433, 500),
            const PresetIconStyle &presetStyle = PresetIconStyle::circle) const;

    public:
        /// @brief This is a static svg for copy-pasting
        const QByteArray styleSvg;

    private:
        /// @brief The icon svg provided by the user
        const QByteArray userIconSvg;

        /// @brief A list of key-value pairs
        /// @details Key will be one of the constant in #C::CK::BK
        const QMap<QString, QString> styles;
    };

    typedef quint32 Slot;

    static Slot
    calcSlot(quint8 pSlot, quint8 tSlot, quint8 rSlot, quint8 subSlot);

    /// @brief A list of buttons
    QHash<Slot, QSharedPointer<ButtonInfo>> buttons;

private:
    void parseConfig(const YAML::Node &config);
};

#endif // CONFIGMANAGER_HPP

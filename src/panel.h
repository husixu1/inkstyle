#ifndef PANEL_H
#define PANEL_H

#include <QPushButton>
#include <QSharedPointer>
#include <QStack>
#include <QWidget>

class Panel : public QWidget {
    Q_OBJECT
private:
    Panel *parentPanel;
    QSharedPointer<Panel> childPanel;

    // Absolute screen position
    QPoint position;

public:
    Panel(Panel *parent = nullptr);

    /// @brief Radius of the main hexagon (edge length)
    qint32 unitLen;
    /// @brief gap/2 between buttons
    qint32 gapLen;

    void redrawMainWindow();

    /// @brief Add buttons that applies style to inkscape objects
    /// @param tSlot theta(angle)-slot, range from 0-5
    /// @param rSlot radius-slot, range from 1-2
    /// @param subSlot sub-slot when angle and radius is defined, \
    ///                range from 0~(rSlot*2)
    /// @details
    /// ```
    /// |           \    tSlot=1    /
    /// |
    /// |             •---•---•---•
    /// |            / \4/3\2/1\0/4\
    /// | tSlot=2   •---•---•---•---•   tSlot=0
    /// |          / \ / \2/1\0/2\3/2\
    /// |         •---•---•---•---•---•
    /// |        / \ / \ /     \1/0\1/0\
    /// |   --  •-b-•-a-•   C   •-a-•-b-•  --
    /// |
    /// | C: center
    /// | a: rSlot=1, b: rSlot=2
    /// | Numbers in cells are subslots
    /// ```
    QPushButton *addStyleButton(quint8 tSlot, quint8 rSlot, quint8 subSlot);

    /// @brief Add button that fills border of the hexagon
    /// @param tSlot theta(angle)-slot, range from 0-5
    QPushButton *addBorderButton(quint8 tSlot);

    void moveEvent(QMoveEvent *event) override;

private slots:
    void addPanel();
};
#endif // PANEL_H

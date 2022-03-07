#ifndef PANEL_H
#    define PANEL_H

#    include "button.h"
#    include "hiddenbutton.h"

#    include <QPushButton>
#    include <QSharedPointer>
#    include <QStack>
#    include <QVector>
#    include <QWeakPointer>
#    include <QWidget>

class Panel : public QWidget {
    Q_OBJECT
private:
    Panel *parentPanel;
    /// @brief The rslot of the parent panel in which this panel resides
    quint8 tSlot;
    QVector<QSharedPointer<Panel>> childPanels;

    QVector<QSharedPointer<HiddenButton>> borderButtons;

    // Absolute screen position
    QPoint position;

public:
    Panel(Panel *parent = nullptr, quint8 tSlot = 0);

    /// @brief Radius of the main hexagon (edge length)
    qint32 unitLen;
    /// @brief gap/2 between buttons
    qint32 gapLen;

    void redrawMainWindow();

    /// @brief Add buttons that applies style to inkscape objects
    /// @param tSlot theta(angle)-slot, 0~5
    /// @param rSlot radius-slot, 1~2
    /// @param subSlot sub-slot when angle and radius is defined, 0~(rSlot*2)
    /// @details
    /// `````````````````````````````````````````
    /// |           \    tSlot=1    /           |
    /// |                                       |
    /// |             •---•---•---•             |
    /// |            / \4/3\2/1\0/4\            |
    /// | tSlot=2   •---•---•---•---•   tSlot=0 |
    /// |          / \ / \2/1\0/2\3/2\          |
    /// |         •---•---•---•---•---•         |
    /// |        / \ / \ /     \1/0\1/0\        |
    /// |   --  •-b-•-a-•   C   •-a-•-b-•  --   |
    /// |                                       |
    /// | C: center                             |
    /// | a: rSlot=1, b: rSlot=2                |
    /// | Numbers in cells are subslots         |
    /// `````````````````````````````````````````
    Button *addStyleButton(quint8 tSlot, quint8 rSlot, quint8 subSlot);

    /// @brief Add button that fills border of the hexagon
    /// @param tSlot theta(angle)-slot, range from 0-5
    HiddenButton *addBorderButton(quint8 tSlot);
    void delBorderButton(quint8 tSlot);

protected:
    void moveEvent(QMoveEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void addPanel(quint8 tSlot);
    void delPanel(quint8 tSlot);
    void copyStyle();
};
#endif // PANEL_H

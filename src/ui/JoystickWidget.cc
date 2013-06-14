#include "JoystickWidget.h"
#include "MainWindow.h"
#include "ui_JoystickWidget.h"
#include "JoystickButton.h"
#include "JoystickAxis.h"
#include <QDebug>
#include <QDesktopWidget>

JoystickWidget::JoystickWidget(JoystickInput* joystick, QWidget *parent) :
    QDialog(parent),
    joystick(joystick),
    m_ui(new Ui::JoystickWidget)
{
    m_ui->setupUi(this);

    // Center the window on the screen.
    QRect position = frameGeometry();
    position.moveCenter(QDesktopWidget().availableGeometry().center());
    move(position.topLeft());

    // Initialize the UI based on the current joystick
    initUI();

    // Watch for button, axis, and hat input events from the joystick.
    connect(this->joystick, SIGNAL(buttonPressed(int)), this, SLOT(joystickButtonPressed(int)));
    connect(this->joystick, SIGNAL(buttonReleased(int)), this, SLOT(joystickButtonReleased(int)));
    connect(this->joystick, SIGNAL(axisValueChanged(int,float)), this, SLOT(updateAxisValue(int,float)));
    connect(this->joystick, SIGNAL(hatDirectionChanged(int,int)), this, SLOT(setHat(int,int)));

    // Update the UI if the joystick changes.
    connect(m_ui->joystickNameComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateUIForJoystick(int)));

    // Enable/disable the UI based on the enable checkbox
    connect(m_ui->enableCheckBox, SIGNAL(toggled(bool)), m_ui->joystickFrame, SLOT(setEnabled(bool)));

    // Update the button label colors based on the current theme and watch for future theme changes.
    styleChanged(MainWindow::instance()->getStyle());
    connect(MainWindow::instance(), SIGNAL(styleChanged(MainWindow::QGC_MAINWINDOW_STYLE)), this, SLOT(styleChanged(MainWindow::QGC_MAINWINDOW_STYLE)));

    // Display the widget above all other windows.
    this->raise();
    this->show();
}

void JoystickWidget::initUI()
{
    // Add the joysticks to the top combobox. They're indexed by their item number.
    // And set the currently-selected combobox item to the current joystick.
    int joysticks = joystick->getNumJoysticks();
    if (joysticks)
    {
        for (int i = 0; i < joysticks; i++)
        {
            m_ui->joystickNameComboBox->addItem(joystick->getJoystickNameById(i));
        }
        m_ui->joystickNameComboBox->setCurrentIndex(joystick->getJoystickID());
        // And if joystick support is enabled, show the UI.
        if (m_ui->enableCheckBox->isChecked())
        {
            m_ui->joystickFrame->setEnabled(true);
        }
    }
    // But if there're no joysticks, just disable everything.
    else
    {
        m_ui->enableCheckBox->setEnabled(false);
        m_ui->joystickNameComboBox->addItem(tr("No joysticks found. Connect and restart QGC to add one."));
        m_ui->joystickNameComboBox->setEnabled(false);
    }

    // Add any missing buttons
    updateUIForJoystick(joystick->getJoystickID());
}

void JoystickWidget::styleChanged(MainWindow::QGC_MAINWINDOW_STYLE newStyle)
{
    if (newStyle == MainWindow::QGC_MAINWINDOW_STYLE_LIGHT)
    {
        buttonLabelColor = QColor(0x73, 0xD9, 0x5D);
    }
    else
    {
        buttonLabelColor = QColor(0x14, 0xC6, 0x14);
    }
}

JoystickWidget::~JoystickWidget()
{
    delete m_ui;
}

void JoystickWidget::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void JoystickWidget::updateUIForJoystick(int id)
{
    // Delete all the old UI elements
    foreach (JoystickButton* b, buttons)
    {
        delete b;
    }
    buttons.clear();
    foreach (JoystickAxis* a, axes)
    {
        delete a;
    }
    axes.clear();

    // Set the JoystickInput to listen to the new joystick instead.
    joystick->setActiveJoystick(id);

    // And add the necessary button displays for this joystick.
    int newButtons = joystick->getJoystickNumButtons();
    if (newButtons)
    {
        m_ui->buttonBox->show();
        for (int i = 0; i < newButtons; i++)
        {
            JoystickButton* button = new JoystickButton(i, m_ui->buttonBox);
            // And make sure we insert BEFORE the vertical spacer.
            m_ui->buttonLayout->insertWidget(i, button);
            buttons.append(button);
        }
    }
    else
    {
        m_ui->buttonBox->hide();
    }

    // Do the same for the axes supported by this joystick.
    int newAxes = joystick->getJoystickNumAxes();
    if (newAxes)
    {
        for (int i = 0; i < newAxes; i++)
        {
            JoystickAxis* axis = new JoystickAxis(i, m_ui->axesBox);
            axis->setValue(joystick->getCurrentValueForAxis(i));
            connect(axis, SIGNAL(mappingChanged(int,JoystickInput::JOYSTICK_INPUT_MAPPING)), this->joystick, SLOT(setAxisMapping(int,JoystickInput::JOYSTICK_INPUT_MAPPING)));
            connect(axis, SIGNAL(inversionChanged(int,bool)), this->joystick, SLOT(setAxisInversion(int,bool)));
            // And make sure we insert BEFORE the vertical spacer.
            m_ui->axesLayout->insertWidget(i, axis);
            axes.append(axis);
        }
    }
    else
    {
        m_ui->buttonBox->hide();
    }
}

void JoystickWidget::updateAxisValue(int axis, float value)
{
    if (axis < axes.size())
    {
        axes.at(axis)->setValue(value);
    }
}

void JoystickWidget::setHat(int x, int y)
{
    m_ui->statusLabel->setText(tr("Hat position: x: %1, y: %2").arg(x).arg(y));
}

void JoystickWidget::joystickButtonPressed(int key)
{
    QString colorStyle = QString("QLabel { background-color: %1;}").arg(buttonLabelColor.name());
    buttons.at(key)->setStyleSheet(colorStyle);
}

void JoystickWidget::joystickButtonReleased(int key)
{
    buttons.at(key)->setStyleSheet("");
}

#include "add_command_dialog.h"
#include "ui_add_command_dialog.h"

AddCommandDialog::AddCommandDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddCommandDialog)
    , type(type)
{
    ui->setupUi(this);

    // Connect the signals
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddCommandDialog::onAccepted);
    connect(ui->presetBox, &QComboBox::currentIndexChanged, this, &AddCommandDialog::loadPreset);

    // Presets
    commandPresetMap.insert(1, {"Reveal map", 0, "REVEAL_MAP_RECT(PLAYER0,125,125,999,999)"});
    commandPresetMap.insert(2, {"Reveal map (console)", 1, "reveal"});
    commandPresetMap.insert(3, {"Add 1,000,000 gold", 0, "ADD_GOLD_TO_PLAYER(PLAYER0,1000000)"});
    commandPresetMap.insert(4, {"Spawn lvl 10 imp", 0, "ADD_CREATURE_TO_LEVEL(PLAYER0,IMP,PLAYER0,1,10,0)"});
    commandPresetMap.insert(5, {"Spawn lvl 10 horny", 0, "ADD_CREATURE_TO_LEVEL(PLAYER0,HORNY,PLAYER0,1,10,0)"});
    commandPresetMap.insert(6, {"Kill all imps", 0, "KILL_CREATURE(ALL_PLAYERS,IMP,ANYWHERE,99999)"});
    commandPresetMap.insert(7, {"Kill all creatures", 0, "KILL_CREATURE(ALL_PLAYERS,ANY_CREATURE,ANYWHERE,99999)"});
    commandPresetMap.insert(8, {"Enable compuchat", 1, "compuchat frequent"});

    // Add presets to preset dropdown box
    ui->presetBox->addItem(""); // empty first one
    for (auto it = commandPresetMap.cbegin(); it != commandPresetMap.cend(); ++it) {
        const CommandPreset &preset = it.value();
        ui->presetBox->addItem(preset.name);
    }

    ui->nameText->setFocus();

    // Show this dialog
    this->show();
}

AddCommandDialog::~AddCommandDialog()
{
    delete ui;
}

void AddCommandDialog::onAccepted()
{
    // Get values
    int type = ui->commandTypeBox->currentIndex();
    QString name= ui->nameText->text();
    QString command = ui->commandText->text();

    // Make sure name and command are set
    if(name.length() == 0 || command.length() == 0){
        QMessageBox::warning(this, "KfxApiTool", "Name and command must be set");
        return;
    }

    // Emit the submit signal and close the dialog
    emit commandSubmitted(name, type, command);

    accept();
}

void AddCommandDialog::loadPreset(int index)
{
    if(index == 0){
        return;
    }

    if (commandPresetMap.contains(index) == false) {
        return;
    }

    // Get preset
    CommandPreset preset = commandPresetMap.value(index);

    // Set preset in the window
    ui->nameText->setText(preset.name);
    ui->commandText->setText(preset.command);
    ui->commandTypeBox->setCurrentIndex(preset.type);

    // Reset presetbox
    ui->presetBox->setCurrentIndex(0);
}

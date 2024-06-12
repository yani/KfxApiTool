#ifndef Q_KFX_SET_VARIABLE_COMPLETER_H
#define Q_KFX_SET_VARIABLE_COMPLETER_H

#include <QCompleter>
#include <QStringList>
#include <QStringListModel>

class QKfxSetVariableCompleter : public QCompleter
{
    Q_OBJECT

public:
    explicit QKfxSetVariableCompleter(QObject *parent = nullptr)
        : QCompleter(parent)
    {
        // List of words for auto-completion
        QStringList wordList = {

            "MONEY", "TOTAL_SCORE",

            "TIMER0", "TIMER1", "TIMER2", "TIMER3", "TIMER4", "TIMER5", "TIMER6", "TIMER7",

            "FLAG0", "FLAG1", "FLAG2", "FLAG3", "FLAG4", "FLAG5", "FLAG6", "FLAG7",

            "CAMPAIGN_FLAG0", "CAMPAIGN_FLAG1", "CAMPAIGN_FLAG2", "CAMPAIGN_FLAG3",
            "CAMPAIGN_FLAG4", "CAMPAIGN_FLAG5", "CAMPAIGN_FLAG6", "CAMPAIGN_FLAG7",
        };

        this->setModel(new QStringListModel(wordList, this));
        this->setCaseSensitivity(Qt::CaseInsensitive);  // Optional: make completer case insensitive
    }
};

#endif // Q_KFX_SET_VARIABLE_COMPLETER_H

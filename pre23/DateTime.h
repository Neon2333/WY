#ifndef DATETIME_H
#define DATETIME_H

#include <QDateTime>
#include <QLineEdit>

QDateTime getDateTimeFromLineEdits(
    QLineEdit* yearEdit, QLineEdit* monEdit, QLineEdit* dayEdit,
    QLineEdit* hourEdit, QLineEdit* minEdit, QLineEdit* secEdit,
    bool* ok = nullptr);

#endif // DATETIME_H

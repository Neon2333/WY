#include "DateTime.h"

QDateTime getDateTimeFromLineEdits(QLineEdit* yearEdit, QLineEdit* monEdit, QLineEdit* dayEdit,
                                   QLineEdit* hourEdit, QLineEdit* minEdit, QLineEdit* secEdit,
                                   bool* ok)
{
    bool valid = false;
    int year, mon, day, hour, min, sec;

    year = yearEdit->text().toInt(&valid);  if (!valid) goto fail;
    mon  = monEdit->text().toInt(&valid);   if (!valid) goto fail;
    day  = dayEdit->text().toInt(&valid);   if (!valid) goto fail;
    hour = hourEdit->text().toInt(&valid);  if (!valid) goto fail;
    min  = minEdit->text().toInt(&valid);   if (!valid) goto fail;
    sec  = secEdit->text().toInt(&valid);   if (!valid) goto fail;

    if (ok) *ok = true;
    return QDateTime(QDate(year, mon, day), QTime(hour, min, sec));

fail:
    if (ok) *ok = false;
    return QDateTime();
}


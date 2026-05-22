#ifndef TO_PDF_H
#define TO_PDF_H

#pragma once

#include <QString>
#include <QDateTime>

void exportTrailToPdf(const QDateTime &start, const QDateTime &end, const QString &filePath);
void exportHisToPdf(const QDateTime &start, const QDateTime &end, const QString &filePath);

void exportHisByUserToPdf(const QString &user, const QString &filePath);
void exportHisByNumberToPdf(const QString &number, const QString &filePath);

void exportTrailByUserToPdf(const QString &user, const QString &filePath);
void exportTrailByNumberToPdf(const QString &number, const QString &filePath);
#endif // TO_PDF_H

#pragma once

#include <memory>

#include <QObject>
#include <QString>

class QTranslator;

namespace car_operator_console {

class LanguageManager final : public QObject {
  Q_OBJECT

 public:
  explicit LanguageManager(const QString &translations_directory = {}, QObject *parent = nullptr);
  ~LanguageManager() override;

  QString language() const;

 public slots:
  bool set_language(const QString &language);

 signals:
  void language_changed(const QString &language);

 private:
  static QString normalize_language(const QString &language);
  static QString default_translations_directory();

  QString translations_directory_;
  QString language_;
  std::unique_ptr<QTranslator> translator_;
};

}  // namespace car_operator_console

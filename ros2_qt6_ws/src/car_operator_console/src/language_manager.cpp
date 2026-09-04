#include "car_operator_console/language_manager.hpp"

#include <stdexcept>
#include <utility>

#include <QCoreApplication>
#include <QDebug>
#include <QSettings>
#include <QTranslator>

#include <ament_index_cpp/get_package_share_directory.hpp>

namespace car_operator_console {

namespace {

constexpr auto kLanguageSettingsKey = "ui/language";
constexpr auto kDefaultLanguage = "en";

}  // namespace

LanguageManager::LanguageManager(const QString &translations_directory, QObject *parent)
    : QObject(parent),
      translations_directory_(translations_directory.isEmpty() ? default_translations_directory()
                                                                : translations_directory) {
  QSettings settings;
  set_language(settings.value(kLanguageSettingsKey, kDefaultLanguage).toString());
}

LanguageManager::~LanguageManager() {
  if (translator_) {
    QCoreApplication::removeTranslator(translator_.get());
  }
}

QString LanguageManager::language() const { return language_; }

bool LanguageManager::set_language(const QString &language) {
  const QString requested_language = normalize_language(language);
  if (translator_) {
    QCoreApplication::removeTranslator(translator_.get());
  }

  auto candidate = std::make_unique<QTranslator>();
  const QString translation_file = QString("car_operator_console_%1").arg(requested_language);
  const QString resource_file = QString(":/i18n/%1.qm").arg(translation_file);
  const bool loaded = candidate->load(resource_file) ||
                      candidate->load(translation_file, translations_directory_);
  if (!loaded && requested_language == "zh_CN") {
    qWarning().noquote() << "Unable to load the Simplified Chinese translation from"
                          << resource_file << "or" << translations_directory_;
    language_ = kDefaultLanguage;
    translator_.reset();
    QSettings().setValue(kLanguageSettingsKey, language_);
    emit language_changed(language_);
    return false;
  }

  language_ = requested_language;
  translator_ = std::move(candidate);
  if (loaded) {
    QCoreApplication::installTranslator(translator_.get());
  }
  QSettings().setValue(kLanguageSettingsKey, language_);
  emit language_changed(language_);
  return true;
}

QString LanguageManager::normalize_language(const QString &language) {
  return language == "zh_CN" ? "zh_CN" : kDefaultLanguage;
}

QString LanguageManager::default_translations_directory() {
  try {
    return QString::fromStdString(
        ament_index_cpp::get_package_share_directory("car_operator_console")) +
           "/translations";
  } catch (const std::runtime_error &) {
    return {};
  }
}

}  // namespace car_operator_console

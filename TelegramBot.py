from telegram import Update, ReplyKeyboardMarkup
from telegram.ext import ApplicationBuilder, CommandHandler, MessageHandler, filters, ContextTypes
import requests
import time

# Константы (укажите свои данные)
BOT_TOKEN = "токен_бота"
DEVICE_IP = "IP_адрес_платы"  # IP адрес вашей платы, пример: http://192.168.8.34

# Клавиатура
keyboard = ReplyKeyboardMarkup(
    keyboard=[["Погнали.."]],
    resize_keyboard=True
)

# Обработка команды /start
async def start(update: Update, context: ContextTypes.DEFAULT_TYPE):
    await update.message.reply_text("Сервер: Выберите действие:", reply_markup=keyboard)

# Повтор запроса до 10 раз
def try_request_with_retries(url, max_retries=10, timeout=3, pause=0.5):
    for attempt in range(1, max_retries + 1):
        try:
            response = requests.get(url, timeout=timeout)
            if response.status_code == 200:
                return True
        except Exception:
            pass
        time.sleep(pause)
    return False

# Обработка текста кнопок
async def handle_message(update: Update, context: ContextTypes.DEFAULT_TYPE):
    text = update.message.text
    if text == "Погнали..":
        await update.message.reply_text("Сервер: Попытка отправить запрос...")
        success = try_request_with_retries(f"{DEVICE_IP}/on")
        if success:
            await update.message.reply_text("Сервер: ✅ Команда отправлена: Погнали..")
        else:
            await update.message.reply_text("Сервер: ❌ Ошибка: не удалось связаться с платой после 10 попыток.")

    else:
        await update.message.reply_text("Сервер: Неизвестная команда.")

# Основной запуск
if __name__ == "__main__":
    app = ApplicationBuilder().token(BOT_TOKEN).build()

    app.add_handler(CommandHandler("start", start))
    app.add_handler(MessageHandler(filters.TEXT & ~filters.COMMAND, handle_message))

    print("Бот запущен...")
    app.run_polling()

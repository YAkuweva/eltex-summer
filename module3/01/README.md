Задание 1 (fork + файлы + каналы)

Сборка
make
Очистка
make clean
Без именованного канала
./01 file1.txt file2.txt
С именованным каналом
./01 -p mypipe file1.txt file2.txt
Тестирование
./test.sh

Структура проекта:
main.c - главная программа с fork()
process.c - функции родительского и дочернего процессов
process.h - заголовочный файл для process.c
file_utils.c - утилиты для работы с файлами
file_utils.h - заголовочный файл для file_utils.c
Makefile - автоматизация сборки
test.sh - скрипт для тестирования
.gitignore - исключает файлы сборки

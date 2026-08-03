#!/bin/bash

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo "Test!!"
echo ""

# Функция для проверки результата
check_result() {
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ PASSED${NC}"
    else
        echo -e "${RED}✗ FAILED${NC}"
    fi
}


echo "Hello from test1" > test1.txt
echo "Hello from test2" > test2.txt
echo "Hello from test3" > test3.txt
touch empty.txt
echo ""

# Тест 1: Без именованного канала, 2 файла
echo "Test 1: without name channel, 2 files"
./01 test1.txt test2.txt
check_result
ls -la test1.txt.copy test2.txt.copy 2>/dev/null
echo ""

# Тест 2: Без именованного канала, 3 файла
echo "Test 2: without name channel, 3 files"
./01 test1.txt test2.txt test3.txt
check_result
ls -la test*.txt.copy 2>/dev/null
echo ""

# Тест 3: С именованным каналом
echo "Test 3: with name channel"
./01 -p mypipe test1.txt test2.txt
check_result
ls -la test1.txt.copy test2.txt.copy 2>/dev/null
echo ""

# Тест 4: Несуществующий файл
echo "Test 4: no real file"
./01 nonexistent.txt 2>error.log
if [ -s error.log ]; then
    echo -e "${GREEN}✓ PASSED (error printed to stderr)${NC}"
    cat error.log
else
    echo -e "${RED}✗ FAILED (no error message)${NC}"
fi
echo ""

# Тест 5: Cуществующих и несуществующих файлов
echo "Test 5: mix real and no real files"
./01 test1.txt nonexistent.txt test2.txt
check_result
ls -la test1.txt.copy test2.txt.copy 2>/dev/null
echo ""

# Тест 6: Пустой файл
echo "Test 6: empty file"
./01 empty.txt
check_result
ls -la empty.txt.copy
cat empty.txt.copy
echo ""

# Тест 7: Файл с пробелами
echo "Test 7: file with spaces"
echo "Content with spaces" > "my file.txt"
./01 "my file.txt"
check_result
ls -la "my file.txt.copy"
cat "my file.txt.copy"
echo ""

# Тест 8: Несколько файлов с именованным каналом
echo "Test 8: files with name channel"
./01 -p mypipe2 test1.txt test2.txt test3.txt
check_result
ls -la test*.txt.copy 2>/dev/null
echo ""

# Очистка
echo "Clean now"
rm -f test*.txt test*.txt.copy empty.txt empty.txt.copy "my file.txt" "my file.txt.copy"
rm -f mypipe mypipe2 error.log
echo "Done!!"

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <random>
#include <iomanip>

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Идентификаторы элементов управления
#define ID_BTN_ROLL     101
#define ID_BTN_ADD      102
#define ID_BTN_DEL      103
#define ID_BTN_RESET    104
#define ID_EDIT_NAME    105
#define ID_EDIT_WEIGHT  106
#define ID_LISTVIEW     107
#define ID_LBL_RESULT   108

struct Student {
    std::wstring name;
    double baseWeight;
    double currWeight;
    int missStreak;
};

std::vector<Student> students;
const std::string DB_FILE = "students_db.txt";
const double ACCELERATION = 0.25; // Коэффициент ускорения роста

HWND hListView, hEditName, hEditWeight, hLblResult;
HFONT hFontMain, hFontBig;

// Конвертация строк для сохранения в UTF-8 файл
std::string toUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string res(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &res[0], size, NULL, NULL);
    return res;
}

std::wstring fromUtf8(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring res(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &res[0], size);
    return res;
}

// Сохранение базы данных
void saveDatabase() {
    std::ofstream file(DB_FILE);
    if (file.is_open()) {
        for (const auto& s : students) {
            file << toUtf8(s.name) << ";" 
                 << s.baseWeight << ";" 
                 << s.currWeight << ";" 
                 << s.missStreak << "\n";
        }
        file.close();
    }
}

// Загрузка базы данных
void loadDatabase() {
    students.clear();
    std::ifstream file(DB_FILE);
    std::string line;

    if (file.is_open()) {
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string nameStr, baseStr, currStr, missStr;

            if (std::getline(ss, nameStr, ';') &&
                std::getline(ss, baseStr, ';') &&
                std::getline(ss, currStr, ';') &&
                std::getline(ss, missStr, ';')) {
                Student s;
                s.name = fromUtf8(nameStr);
                s.baseWeight = std::stod(baseStr);
                s.currWeight = std::stod(currStr);
                s.missStreak = std::stoi(missStr);
                students.push_back(s);
            }
        }
        file.close();
    }

    // Если база пустая, добавим пару примеров
    if (students.empty()) {
        students = {
            {L"Иванов Иван", 1.0, 1.0, 0},
            {L"Смирнова Анна", 1.5, 1.5, 0},
            {L"Петров Алексей", 0.8, 0.8, 0}
        };
        saveDatabase();
    }
}

// Обновление таблицы на экране
void updateListView() {
    ListView_DeleteAllItems(hListView);
    for (size_t i = 0; i < students.size(); ++i) {
        LVITEM lvi = { 0 };
        lvi.mask = LVIF_TEXT;
        lvi.iItem = (int)i;
        lvi.iSubItem = 0;
        lvi.pszText = (LPWSTR)students[i].name.c_str();
        ListView_InsertItem(hListView, &lvi);

        wchar_t buf[64];
        swprintf(buf, 64, L"%.2f", students[i].baseWeight);
        ListView_SetItemText(hListView, (int)i, 1, buf);

        swprintf(buf, 64, L"%.2f", students[i].currWeight);
        ListView_SetItemText(hListView, (int)i, 2, buf);

        swprintf(buf, 64, L"%d", students[i].missStreak);
        ListView_SetItemText(hListView, (int)i, 3, buf);
    }
}

// Логика выбора ученика
void rollStudent(HWND hwnd) {
    if (students.empty()) {
        MessageBox(hwnd, L"Добавьте хотя бы одного ученика!", L"Внимание", MB_OK | MB_ICONWARNING);
        return;
    }

    std::vector<double> weights;
    for (const auto& s : students) {
        weights.push_back(s.currWeight);
    }

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::discrete_distribution<> dist(weights.begin(), weights.end());
    
    int chosenIndex = dist(gen);
    std::wstring chosenName = students[chosenIndex].name;

    // Пересчет коэффициентов
    for (size_t i = 0; i < students.size(); ++i) {
        if ((int)i == chosenIndex) {
            students[i].missStreak = 0;
            students[i].currWeight = students[i].baseWeight;
        } else {
            students[i].missStreak++;
            double bonus = students[i].baseWeight * (students[i].missStreak * ACCELERATION);
            students[i].currWeight += bonus;
        }
    }

    saveDatabase();
    updateListView();

    std::wstring resText = L"Отвечает: " + chosenName;
    SetWindowText(hLblResult, resText.c_str());
}

// Обработчик событий окна
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            hFontMain = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                   OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            hFontBig = CreateFont(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

            // Поля ввода нового ученика
            HWND lbl1 = CreateWindow(L"STATIC", L"Имя ученика:", WS_VISIBLE | WS_CHILD, 20, 20, 120, 25, hwnd, NULL, NULL, NULL);
            hEditName = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 20, 45, 220, 28, hwnd, (HMENU)ID_EDIT_NAME, NULL, NULL);

            HWND lbl2 = CreateWindow(L"STATIC", L"Баз. коэфф.:", WS_VISIBLE | WS_CHILD, 250, 20, 100, 25, hwnd, NULL, NULL, NULL);
            hEditWeight = CreateWindow(L"EDIT", L"1.0", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 250, 45, 80, 28, hwnd, (HMENU)ID_EDIT_WEIGHT, NULL, NULL);

            HWND btnAdd = CreateWindow(L"BUTTON", L"Добавить", WS_VISIBLE | WS_CHILD, 345, 45, 110, 28, hwnd, (HMENU)ID_BTN_ADD, NULL, NULL);
            HWND btnDel = CreateWindow(L"BUTTON", L"Удалить выбр.", WS_VISIBLE | WS_CHILD, 465, 45, 125, 28, hwnd, (HMENU)ID_BTN_DEL, NULL, NULL);

            // Таблица (ListView)
            hListView = CreateWindow(WC_LISTVIEW, L"", WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SINGLESEL,
                                     20, 90, 570, 260, hwnd, (HMENU)ID_LISTVIEW, NULL, NULL);
            ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

            LVCOLUMN lvc = { 0 };
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            
            lvc.pszText = (LPWSTR)L"Ученик"; lvc.cx = 230; ListView_InsertColumn(hListView, 0, &lvc);
            lvc.pszText = (LPWSTR)L"Базовый вес"; lvc.cx = 110; ListView_InsertColumn(hListView, 1, &lvc);
            lvc.pszText = (LPWSTR)L"Текущий вес"; lvc.cx = 110; ListView_InsertColumn(hListView, 2, &lvc);
            lvc.pszText = (LPWSTR)L"Пропусков"; lvc.cx = 100; ListView_InsertColumn(hListView, 3, &lvc);

            // Кнопки действия и результат
            HWND btnRoll = CreateWindow(L"BUTTON", L"ВЫБРАТЬ УЧЕНИКА", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 20, 365, 220, 45, hwnd, (HMENU)ID_BTN_ROLL, NULL, NULL);
            HWND btnReset = CreateWindow(L"BUTTON", L"Сбросить шансы", WS_VISIBLE | WS_CHILD, 450, 365, 140, 45, hwnd, (HMENU)ID_BTN_RESET, NULL, NULL);

            hLblResult = CreateWindow(L"STATIC", L"Нажмите кнопку для выбора...", WS_VISIBLE | WS_CHILD | SS_CENTER,
                                      20, 425, 570, 40, hwnd, (HMENU)ID_LBL_RESULT, NULL, NULL);

            // Применяем шрифты
            SendMessage(lbl1, WM_SETFONT, (WPARAM)hFontMain, TRUE);
            SendMessage(hEditName, WM_SETFONT, (WPARAM)hFontMain, TRUE);
            SendMessage(lbl2, WM_SETFONT, (WPARAM)hFontMain, TRUE);
            SendMessage(hEditWeight, WM_SETFONT, (WPARAM)hFontMain, TRUE);
            SendMessage(btnAdd, WM_SETFONT, (WPARAM)hFontMain, TRUE);
            SendMessage(btnDel, WM_SETFONT, (WPARAM)hFontMain, TRUE);
            SendMessage(hListView, WM_SETFONT, (WPARAM)hFontMain, TRUE);
            SendMessage(btnRoll, WM_SETFONT, (WPARAM)hFontMain, TRUE);
            SendMessage(btnReset, WM_SETFONT, (WPARAM)hFontMain, TRUE);
            SendMessage(hLblResult, WM_SETFONT, (WPARAM)hFontBig, TRUE);

            loadDatabase();
            updateListView();
            break;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == ID_BTN_ADD) {
                wchar_t nameBuf[128], weightBuf[32];
                GetWindowText(hEditName, nameBuf, 128);
                GetWindowText(hEditWeight, weightBuf, 32);

                std::wstring name(nameBuf);
                if (name.empty()) {
                    MessageBox(hwnd, L"Введите имя ученика!", L"Ошибка", MB_OK | MB_ICONERROR);
                    break;
                }

                // Замена запятой на точку для корректного чтения числа
                std::wstring wStr(weightBuf);
                for (auto& c : wStr) if (c == L',') c = L'.';
                
                double w = 1.0;
                try {
                    w = std::stod(wStr);
                    if (w <= 0) w = 0.1;
                } catch (...) {
                    w = 1.0;
                }

                students.push_back({name, w, w, 0});
                saveDatabase();
                updateListView();
                SetWindowText(hEditName, L"");
            }
            else if (id == ID_BTN_DEL) {
                int sel = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
                if (sel != -1) {
                    students.erase(students.begin() + sel);
                    saveDatabase();
                    updateListView();
                } else {
                    MessageBox(hwnd, L"Выберите ученика в таблице для удаления.", L"Внимание", MB_OK | MB_ICONINFORMATION);
                }
            }
            else if (id == ID_BTN_ROLL) {
                rollStudent(hwnd);
            }
            else if (id == ID_BTN_RESET) {
                for (auto& s : students) {
                    s.currWeight = s.baseWeight;
                    s.missStreak = 0;
                }
                saveDatabase();
                updateListView();
                SetWindowText(hLblResult, L"Шансы сброшены к базовым");
            }
            break;
        }

        case WM_DESTROY:
            DeleteObject(hFontMain);
            DeleteObject(hFontBig);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"StudentRandomizerClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindow(wc.lpszClassName, L"Генератор опроса учеников",
                             WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                             CW_USEDEFAULT, CW_USEDEFAULT, 625, 520, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdio.h>

#include <implot.h>
#include <implot_internal.h>

#include <vector>
#include <cmath>

#include <windows.h>
#include <string>

#include <ComPort.h>

enum
{
    END = 192,
    ESC = 219,
    ESC_END = 220,
    ESC_ESC = 221
};

// SLIP state machine
struct slip
{
    unsigned char *buf; // Buffer for the network mode
    size_t size;        // Buffer size
    size_t len;         // Number of currently buffered bytes
    int mode;           // Operation mode. 0 - serial, 1 - network
    unsigned char prev; // Previously read character
};

struct ctx
{
    struct slip slip; // SLIP state machine
    const char *baud; // Baud rate, e.g. "115200"
    const char *port; // Serial port, e.g. "/dev/ttyUSB0"
    const char *fpar; // Flash params, e.g. "0x220"
    const char *fspi; // Flash SPI pins: CLK,Q,D,HD,CS. E.g. "6,17,8,11,16"
    bool verbose;     // Hexdump serial comms
    int fd;           // Serial port file descriptor
};

// Process incoming byte `c`.
// In serial mode, do nothing, return 1.
// In network mode, append a byte to the `buf` and increment `len`.
// Return size of the buffered packet when switching to serial mode, or 0
static size_t slip_recv(unsigned char c, struct slip *slip)
{
    size_t res = 0;
    if (slip->mode)
    {
        if (slip->prev == ESC && c == ESC_END)
        {
            slip->buf[slip->len++] = END;
        }
        else if (slip->prev == ESC && c == ESC_ESC)
        {
            slip->buf[slip->len++] = ESC;
        }
        else if (c == END)
        {
            res = slip->len;
        }
        else if (c != ESC)
        {
            slip->buf[slip->len++] = c;
        }
        if (slip->len >= slip->size)
            slip->len = 0; // Silent overflow
    }
    slip->prev = c;
    // The "END" character flips the mode
    if (c == END)
        slip->len = 0, slip->mode = !slip->mode;
    return res;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

void RenderBottomMenu()
{
    // Получаем размеры экрана (предполагается, что у вас есть доступ к этим данным)
    ImVec2 screenSize = ImGui::GetIO().DisplaySize; // Размер окна приложения
    float menuHeight = 200.0f;                      // Высота нижнего меню

    // Устанавливаем позицию и размер окна
    ImGui::SetNextWindowPos(ImVec2(0, screenSize.y - menuHeight)); // Позиция внизу экрана
    ImGui::SetNextWindowSize(ImVec2(screenSize.x, menuHeight));    // Ширина = экран, высота = фиксированная

    // Начинаем окно с флагами
    ImGui::Begin("BottomMenu", nullptr,
                 ImGuiWindowFlags_NoTitleBar |          // Без заголовка
                     ImGuiWindowFlags_NoResize |        // Нельзя менять размер
                     ImGuiWindowFlags_NoMove |          // Нельзя двигать
                     ImGuiWindowFlags_NoScrollbar |     // Без прокрутки
                     ImGuiWindowFlags_NoSavedSettings); // Не сохранять настройки

    // Добавляем элементы в меню
    ImGui::Text("Bottom Menu: ");
    ImGui::SameLine(); // Располагаем следующий элемент на той же строке
    if (ImGui::Button("button 1"))
    {
        // Действие для кнопки 1
    }
    ImGui::SameLine();
    if (ImGui::Button("button 2"))
    {
        // Действие для кнопки 2
    }
    ImGui::SameLine();
    if (ImGui::Button("button 3"))
    {
        // Действие для кнопки 3
    }

    // Завершаем окно
    ImGui::End();
}

// utility structure for realtime plot
struct ScrollingBuffer
{
    int MaxSize;
    int Offset;
    ImVector<ImVec2> Data;
    ScrollingBuffer(int max_size = 6000)
    {
        MaxSize = max_size;
        Offset = 0;
        Data.reserve(MaxSize);
    }
    void AddPoint(float x, float y)
    {
        if (Data.size() < MaxSize)
            Data.push_back(ImVec2(x, y));
        else
        {
            Data[Offset] = ImVec2(x, y);
            Offset = (Offset + 1) % MaxSize;
        }
    }
    void Erase()
    {
        if (Data.size() > 0)
        {
            Data.shrink(0);
            Offset = 0;
        }
    }
};

void RenderGraphs()
{
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;                     // Размер экрана
    ImGui::SetNextWindowPos(ImVec2(0, 20));                             // Позиция: x=0, y=высота меню
    ImGui::SetNextWindowSize(ImVec2(screenSize.x, screenSize.y - 250)); // Размер: ширина экрана, высота = экран - меню

    // Создаём окно под меню
    ImGui::Begin("Окно под меню", nullptr,
                 ImGuiWindowFlags_NoTitleBar |   // Без заголовка
                     ImGuiWindowFlags_NoResize | // Нельзя менять размер
                     ImGuiWindowFlags_NoMove);   // Нельзя двигать

    static ScrollingBuffer sdata2;
    ImVec2 mouse = ImGui::GetMousePos();
    static float t = 0;
    t += ImGui::GetIO().DeltaTime;
    sdata2.AddPoint(t, mouse.y * 0.0005f);

    static float history = 10.0f;
    ImGui::SliderFloat("History", &history, 1, 30, "%.1f s");

    static ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels;

    if (ImPlot::BeginPlot("##Scrolling", ImVec2(-1, 500)))
    {
        ImPlot::SetupAxes(nullptr, nullptr, flags, flags);
        ImPlot::SetupAxisLimits(ImAxis_X1, t - history, t, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1);
        ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
        ImPlot::PushStyleVar(ImPlotStyleVar_LineWeight, 2.0f);
        ImPlot::SetupAxes("x", "y");
        ImPlot::PlotLine("Mouse Y", &sdata2.Data[0].x, &sdata2.Data[0].y, sdata2.Data.size(), 0, sdata2.Offset, 2 * sizeof(float));
        ImPlot::EndPlot();
    }
    ImGui::End();
}

void OnDataReceive(char data)
{
}

class Application
{

private:
    std::string openned_com_name = "No opened COM";
    ComPort COM;

    uint8_t slipbuf[32 * 1024]; // Buffer for SLIP context
    struct ctx ctx = {0};       // Program context

public:
    Application() : window(nullptr)
    {
        ctx.slip.buf = slipbuf;          // Set SLIP context - buffer
        ctx.slip.size = sizeof(slipbuf); // Buffer size

        if (!glfwInit())
        {
            printf("Failed to initialize GLFW\n");
            return;
        }

        window = glfwCreateWindow(800, 600, "ImGui + GLFW Example", nullptr, nullptr);
        if (!window)
        {
            printf("Failed to create GLFW window\n");
            glfwTerminate();
            return;
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();

        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        ImGui::StyleColorsDark();

        if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
        {
            printf("Failed to initialize ImGui GLFW backend\n");
            return;
        }
        if (!ImGui_ImplOpenGL3_Init("#version 130"))
        {
            printf("Failed to initialize ImGui OpenGL backend\n");
            return;
        }
    }

    ~Application()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        if (window)
        {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }

    void slip_send(const void *buf, size_t len)
    {
        const unsigned char *p = (unsigned char *)buf;
        size_t i;
        COM.Write(END);
        for (i = 0; i < len; i++)
        {
            if (p[i] == END)
            {
                COM.Write(ESC);
                COM.Write(ESC_END);
            }
            else if (p[i] == ESC)
            {
                COM.Write(ESC);
                COM.Write(ESC_ESC);
            }
            else
            {
                COM.Write(p[i]);
            }
        }
        COM.Write(END);
    }

    int run()
    {
        if (!window)
        {
            return -1;
        }

        bool show_demo_window = false;
        bool is_red_background = false;
        float slider_value = 0.5f;   // Значение для слайдера
        bool checkbox_value = false; // Значение для чекбокса

        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Главное меню
            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("New"))
                    {
                        printf("New file selected\n");
                    }
                    if (ImGui::MenuItem("Exit", "Ctrl+Q"))
                    {
                        glfwSetWindowShouldClose(window, true);
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("View"))
                {
                    ImGui::MenuItem("Demo Window", nullptr, &show_demo_window);
                    ImGui::MenuItem("Red Background", nullptr, &is_red_background);
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu(openned_com_name.c_str()))
                {
                    static std::vector<std::string> com_ports;

                    if (COM.is_opened())
                    {
                        if (ImGui::Button("Close"))
                            COM.close();
                    }
                    else
                    {
                        if (ImGui::Button("Update"))
                            com_ports = COM.GetCOMPortsFromRegistry();

                        if (com_ports.empty())
                        {
                            ImGui::Text("No com ports");
                        }
                        else
                        {
                            for (const auto &port : com_ports)
                            {
                                if (ImGui::CollapsingHeader(port.c_str()))
                                {
                                    const char *items[] = {
                                        "9600",
                                        "115200",
                                    };
                                    static int item_current = 0;

                                    if (ImGui::Combo("combo", &item_current, items, IM_ARRAYSIZE(items)))
                                    {
                                        if (COM.open("\\\\.\\" + port, atoi(items[item_current]), OnDataReceive))
                                        {
                                            openned_com_name = port;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Test write"))
                {
                    if (ImGui::Button("send"))
                    {
                        slip_send("AB\0x25", 3);
                    }

                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            RenderBottomMenu();
            RenderGraphs();
            // Установка начальной позиции (опционально)

            // Отображение демо-окна, если выбрано
            if (show_demo_window)
            {
                ImGui::ShowDemoWindow(&show_demo_window);
            }

            // Рендеринг
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);

            if (is_red_background)
            {
                glClearColor(0.5f, 0.0f, 0.0f, 1.0f);
            }
            else
            {
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            }
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
        }

        return 0;
    }

private:
    GLFWwindow *window;
};

int main()
{

    Application app;
    return app.run();
}

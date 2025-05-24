#include "Kaey/Parser/KaeyParser.hpp"
#include "Kaey/Others/GraphvizPlotter.hpp"

#include <opencv2/opencv.hpp>

//x64 double -> n >= 27, float -> n >= 29
//x86 double -> n >= 32, float -> n >= 33

template<class Fn>
void CVSetMouseCallback(const cv::String& winname, Fn fn)
{
    auto ptr = new Fn(std::move(fn));
    cv::setMouseCallback(winname, +[](int event, int x, int y, int flags, void* userdata) -> void { std::invoke(*(Fn*)userdata, event, x, y, flags); }, ptr);
}

int main(int argc, char* argv[])
{
    using Kaey::print;
    using Kaey::println;
    using namespace Kaey::Lexer;
    using namespace Kaey::Parser;
    try
    {
        if (argc <= 1)
        {
            puts("error: no input files");
            return -1;
        }
        std::filesystem::path path{ argv[1] };
        if (!exists(path) || !is_regular_file(path))
        {
            puts("error: file not found");
            return -1;
        }

        auto ss = StringStream::FromFile(path);
        auto ts = TokenStream(ss);
        for (auto tk = ts.Consume(); !tk->Is<Eof>(); tk = ts.Consume())
            print("{} ", tk->Text());
        puts("\n\n");
        ts.Seek(0, SeekOrigin::Begin);
        auto mod = Module::Parse("Module", ts);
        {
            Graphviz::GraphvizPlotter plotter;
            std::ofstream ofs("tree.gv");
            plotter.Plot(ofs, &mod);
            ofs.close();
        }
        std::system(R"(dot tree.gv | dot -T png -o tree.png)");

        auto largeImage = cv::imread("tree.png");

        namedWindow("Module", cv::WINDOW_AUTOSIZE);

        int winW = 1280;
        int winH = 720;
        if (winW >= largeImage.cols)
            winW = largeImage.cols - 1;
        if (winH >= largeImage.rows)
            winH = largeImage.rows - 1;

        pair<int, int> scroll;
        CVSetMouseCallback("Module", [&](int event, int x, int y, int flags) -> void
        {
            static bool down = false;
            static pair<int, int> pos;
            switch (event)
            {
            case cv::EVENT_LBUTTONDOWN:
            {
                auto& [sx, sy] = scroll;
                down = true;
                pos = { sx + x, sy + y };
            }break;
            case cv::EVENT_LBUTTONUP:
                down = false;
                break;
            case cv::EVENT_MOUSEMOVE:
            {
                if (!down)
                    break;
                auto& [px, py] = pos;
                auto& [sx, sy] = scroll;
                sx = std::clamp(px - x, 0, largeImage.cols - 1 - winW);
                sy = std::clamp(py - y, 0, largeImage.rows - 1 - winH);
            }break;
            default:
                break;
            }
        });
        while (cv::waitKey(1) != 'q')
        {
            auto& [sx, sy] = scroll;
            imshow("Module", largeImage(cv::Rect(sx, sy, winW, winH)));
        }
    }
    catch (std::ios::failure& e)
    {
        println("error: couldn't open file! {}\n", e.what());
    }
    catch (std::runtime_error& e)
    {
        puts(e.what());
    }
    return -1; 
}

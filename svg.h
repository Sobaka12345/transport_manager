#ifndef SVG_H
#define SVG_H

#include <iostream>
#include <optional>
#include <variant>
#include <iomanip>
#include <memory>
#include <vector>
#include <string>

#include <cmath>

#include "json.h"


namespace Svg {

struct Point
{
    double x = 0.0;
    double y = 0.0;
};

struct Rgb
{
    ushort red;
    ushort green;
    ushort blue;
    std::optional<double> alpha;

    Rgb(ushort r, ushort g, ushort b) :
        red(r),
        green(g),
        blue(b)
    {}

    Rgb(ushort r, ushort g, ushort b, double a) :
        red(r),
        green(g),
        blue(b),
        alpha(a)
    {}

    friend std::ostream& operator<<(std::ostream& stream, const Rgb& val)
    {
        if(val.alpha.has_value())
        {
            stream << "rgba(" << val.red << ','
                   << val.green << ','
                   << val.blue << ','
                   << val.alpha.value() << ')';
        } else
        {
            stream << "rgb(" << val.red << ','
                   << val.green << ','
                   << val.blue << ')';
        }

        return stream;
    }
};

class Color
{
    std::optional<std::variant<std::string, Rgb>> color_;

public:
    Color() = default;

    Color(const std::vector<Json::Node>& rgb)
    {
        if(rgb.size() == 4)
        {
            color_ = Rgb(ushort(rgb[0].AsInt()),
                         ushort(rgb[1].AsInt()),
                         ushort(rgb[2].AsInt()),
                         rgb[3].AsDouble());

        } else
        {
            color_ = Rgb(ushort(rgb[0].AsInt()),
                         ushort(rgb[1].AsInt()),
                         ushort(rgb[2].AsInt()));
        }
    }

    Color(std::string color) :
        color_(std::move(color))
    {}

    Color(Rgb color) :
        color_(std::move(color))
    {}

    friend std::ostream& operator<<(std::ostream& stream, const Color& val)
    {
        const auto getVal = [&stream](const auto& var) { stream << var; };
        if(val.color_.has_value())
        {
            std::visit(getVal, val.color_.value());
        } else
            stream << "none";

        return stream;
    }
};

static const Color NoneColor;

class FigureInterface
{
public:
    virtual ~FigureInterface() {};
    virtual void print(std::ostream&) = 0;
    virtual void printRaw(std::ostream&) = 0;
};

template <typename Derived>
class Figure : public FigureInterface
{
protected:
    Color fillColor_, strokeColor_;
    double strokeWidth_;
    std::optional<std::string> strokeLineCap_, strokeLineJoin_;

    Figure() :
        fillColor_(NoneColor),
        strokeColor_(NoneColor),
        strokeWidth_(1.0)
    {}

    void printBase(std::ostream& stream) const
    {
        stream << "fill=\"" << fillColor_ << "\" "
               << "stroke=\"" << strokeColor_ << "\" "
               << "stroke-width=\"" << strokeWidth_ << "\" ";
        if(strokeLineCap_.has_value())
            stream << "stroke-linecap=\"" << strokeLineCap_.value() << "\" ";
        if(strokeLineJoin_.has_value())
            stream << "stroke-linejoin=\"" << strokeLineJoin_.value() << "\" ";
    }

    void printBaseRaw(std::ostream& stream) const
    {
        stream << "fill=\\\"" << fillColor_ << "\\\" "
               << "stroke=\\\"" << strokeColor_ << "\\\" "
               << "stroke-width=\\\"" << strokeWidth_ << "\\\" ";
        if(strokeLineCap_.has_value())
            stream << "stroke-linecap=\\\"" << strokeLineCap_.value() << "\\\" ";
        if(strokeLineJoin_.has_value())
            stream << "stroke-linejoin=\\\"" << strokeLineJoin_.value() << "\\\" ";
    }

public:
    virtual void print(std::ostream& stream) override
    {
         static_cast<Derived*>(this)->print(stream);
    }

    virtual void printRaw(std::ostream& stream) override
    {
         static_cast<Derived*>(this)->printRaw(stream);
    }


    Derived& SetFillColor(const Color& color)
    {
        fillColor_ = color;

        return *static_cast<Derived*>(this);
    }

    Derived& SetFillColor(std::string color)
    {
        fillColor_ = Color(move(color));

        return *static_cast<Derived*>(this);
    }

    Derived& SetStrokeColor(const Color& color)
    {
        strokeColor_ = color;

        return *static_cast<Derived*>(this);
    }

    Derived& SetStrokeWidth(double width)
    {
        strokeWidth_ = width;

        return *static_cast<Derived*>(this);
    }

    Derived& SetStrokeLineCap(const std::string& lineCap)
    {
        strokeLineCap_ = lineCap;

        return *static_cast<Derived*>(this);
    }

    Derived& SetStrokeLineJoin(const std::string lineJoin)
    {
        strokeLineJoin_ = lineJoin;

        return *static_cast<Derived*>(this);
    }
};
class Circle: public Figure<Circle>
{
protected:
    double centerX_, centerY_, radius_;

public:
    Circle() :
        Figure(),
        centerX_(0.0),
        centerY_(0.0)
    {}

    Circle& SetCenter(Point center)
    {
        centerX_ = center.x;
        centerY_ = center.y;

        return *this;
    }

    Circle& SetRadius(double r)
    {
        radius_ = r;

        return *this;
    }

    void print(std::ostream &stream) override
    {
        stream << "<circle ";
        stream << "cx=\"" << centerX_ << "\" "
               << "cy=\"" << centerY_ << "\" "
               << "r=\"" << radius_ << "\" ";
        printBase(stream);
        stream << "/>";
    }
    void printRaw(std::ostream &stream) override
    {
        stream << "<circle ";
        stream << "cx=\\\"" << centerX_ << "\\\" "
               << "cy=\\\"" << centerY_ << "\\\" "
               << "r=\\\"" << radius_ << "\\\" ";
        printBaseRaw(stream);
        stream << "/>";
    }

};

class Polyline: public Figure<Polyline>
{
protected:
    std::vector<Point> points_;

public:
    Polyline() :
        Figure()
    {}

    Polyline& AddPoint(Point point)
    {
        points_.push_back(std::move(point));

        return *this;
    }

    void print(std::ostream &stream) override
    {
        stream << "<polyline points=\"";
        for(const auto& point : points_)
            stream << point.x << "," << point.y << " ";
        stream << "\" ";
        printBase(stream);
        stream << "/>";
    }

    void printRaw(std::ostream &stream) override
    {
        stream << "<polyline points=\\\"";
        for(const auto& point : points_)
            stream << point.x << "," << point.y << " ";
        stream << "\\\" ";
        printBaseRaw(stream);
        stream << "/>";
    }
};

class Text: public Figure<Text>
{
protected:
    Point point_, offset_;
    uint32_t fontSize_;
    std::optional<std::string> fontFamily_, fontWeight_;
    std::string data_;

public:
    Text() :
        Figure(),
        fontSize_(1)
    {}

    Text& SetPoint(Point point)
    {
        point_ = std::move(point);

        return *this;
    }

    Text& SetOffset(Point offset)
    {
        offset_ = std::move(offset);

        return *this;
    }

    Text& SetFontSize(uint32_t size)
    {
        fontSize_ = size;

        return *this;
    }

    Text& SetFontFamily(const std::string& family)
    {
        fontFamily_ = family;

        return *this;
    }

    Text& SetData(const std::string& data)
    {
        data_ = data;

        return *this;
    }

    Text& SetFontWeight(const std::string& weight)
    {
        fontWeight_ = weight;

        return *this;
    }

    void print(std::ostream &stream) override
    {
        stream << "<text ";
        stream << "x=\"" << point_.x << "\" "
               << "y=\"" << point_.y << "\" "
               << "dx=\"" << offset_.x << "\" "
               << "dy=\"" << offset_.y << "\" "
               << "font-size=\"" << fontSize_ << "\" ";

        printBase(stream);
        if(fontFamily_.has_value())
            stream << "font-family=\"" << fontFamily_.value() << "\" ";
        if(fontWeight_.has_value())
            stream << "font-weight=\"" << fontWeight_.value() << "\" ";
        stream << ">" << data_ << "</text>";
    }

    void printRaw(std::ostream &stream) override
    {
        stream << "<text ";
        stream << "x=\\\"" << point_.x << "\\\" "
               << "y=\\\"" << point_.y << "\\\" "
               << "dx=\\\"" << offset_.x << "\\\" "
               << "dy=\\\"" << offset_.y << "\\\" "
               << "font-size=\\\"" << fontSize_ << "\\\" ";
        printBaseRaw(stream);
        if(fontFamily_.has_value())
            stream << "font-family=\\\"" << fontFamily_.value() << "\\\" ";
        if(fontWeight_.has_value())
            stream << "font-weight=\\\"" << fontWeight_.value() << "\\\" ";
        stream << ">" << data_ << "</text>";
    }
};

class Document
{
    std::vector<std::unique_ptr<FigureInterface>> figures_;

public:
    Document()
    {}

    template <typename T>
    void Add(T figure)
    {
        figures_.push_back(std::make_unique<T>(std::move(figure)));
    }

    void Render(std::ostream& out, bool raw = false) const
    {

        if(raw)
        {
            out << "<?xml version=\\\"1.0\\\" encoding=\\\"UTF-8\\\" ?>"
                   "<svg xmlns=\\\"http://www.w3.org/2000/svg\\\" version=\\\"1.1\\\">";
            for(const auto& figure: figures_)
                figure->printRaw(out);
        } else
        {
            out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"
                   "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">";
            for(const auto& figure: figures_)
                figure->print(out);
        }

        out << "</svg>";
    }
};

}

#endif // SVG_H

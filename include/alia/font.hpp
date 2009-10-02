#ifndef ALIA_FONT_HPP
#define ALIA_FONT_HPP

#include <string>
#include <boost/shared_ptr.hpp>
#include <alia/forward.hpp>

namespace alia {

struct font
{
    static unsigned const
        DEFAULT_STYLE = 0,
        BOLD = 1,
        ITALIC = 2,
        UNDERLINED = 4;

    // Used to store system-specific info that can't be captured by the generic
    // font structure.
    struct system_info
    {
        virtual ~system_info() {}
        virtual system_info* adjust_style(unsigned old_style,
            unsigned new_style) const = 0;
        virtual system_info* adjust_size(float size) const = 0;
    };

    font() {}

    font(std::string const& name, float size, unsigned style = DEFAULT_STYLE,
        float line_height = 1)
      : name_(name), size_(size), style_(style), line_height_(line_height)
    {}

    font(std::string const& name, float size,
        boost::shared_ptr<system_info> info,
        unsigned style = DEFAULT_STYLE,
        float line_height = 1)
      : name_(name), size_(size), style_(style), info_(info),
        line_height_(line_height)
    {}

    unsigned get_style() const { return style_; }

    bool is_bold() const { return (style_ & BOLD) != 0; }
    bool is_italic() const { return (style_ & ITALIC) != 0; }
    bool is_underlined() const { return (style_ & UNDERLINED) != 0; }

    std::string const& get_name() const { return name_; }

    float get_size() const { return size_; }
    float get_line_height() const { return line_height_; }

    system_info const* get_info() const { return info_.get(); }

    void set_style(unsigned style)
    {
        if (info_)
        {
            system_info* new_info = info_->adjust_style(style_, style);
            info_.reset(new_info);
        }
        style_ = style;
    }

    void set_size(float size)
    {
        if (info_)
        {
            system_info* new_info = info_->adjust_size(size);
            info_.reset(new_info);
        }
        size_ = size;
    }

    // TODO: Involve the system info in the comparison operators.

    bool operator==(font const& other) const
    {
        return name_ == other.name_ && size_ == other.size_ &&
            style_ == other.style_;
    }
    bool operator!=(font const& other) const
    {
        return name_ != other.name_ || size_ != other.size_ ||
            style_ != other.style_;
    }
    bool operator<(font const& other) const
    {
        return name_ < other.name_ || (name_ == other.name_ &&
            (size_ < other.size_ || (size_ == other.size_ &&
            style_ < other.style_)));
    }

 private:
    std::string name_;
    float size_;
    unsigned style_;
    boost::shared_ptr<system_info> info_;
    float line_height_;
};

struct font_metrics
{
    int height;
    int ascent;
    int descent;
    int average_width;
    int row_gap;
    int overhang;
};

font_metrics const& get_font_metrics(context& ctx, font const& f);

void set_font_size_adjustment(context& ctx, float adjustment);

font shrink(font const& f, float amount);
font enlarge(font const& f, float amount);

font add_style(font const& f, unsigned style);

}

#endif

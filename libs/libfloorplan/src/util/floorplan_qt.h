#pragma once

#ifdef USE_QT

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QResizeEvent>
#include <cstddef>
#include <string>
#include <vector>

#include "tile_qor.h"

namespace arcxpress {

class FloorplanGraphicsView : public QGraphicsView {
 public:
  struct ShapeRect {
    std::string instance;
    std::string module;
    float x = 0.f;
    float y = 0.f;
    float w = 0.f;
    float h = 0.f;
  };

  explicit FloorplanGraphicsView(QWidget* parent = nullptr);

  void render_layout(const std::vector<ShapeRect>& rects, float canvas_w, float canvas_h,
                     float canvas_pad, size_t precision, const std::vector<bool>& overlap_flags);

  static size_t analyze_tile_geometry(const std::vector<ShapeRect>& rects,
                                      std::vector<bool>& is_overlapping, float& min_gap_x,
                                      float& min_gap_y);

 protected:
  void resizeEvent(QResizeEvent* event) override;

 private:
  static bool has_positive_overlap(const ShapeRect& a, const ShapeRect& b, float eps);
  void redraw_scene();

  QGraphicsScene* scene_ = nullptr;
  std::vector<ShapeRect> rects_;
  std::vector<bool> overlap_flags_;
  float canvas_w_ = 0.f;
  float canvas_h_ = 0.f;
  float canvas_pad_ = 0.f;
  size_t precision_ = 2;
};

int show_floorplan_data_window(const std::string& shaping_rpt, const TileQor& tile_qor,
                               const float& shrink_factor, const size_t& precision);

}  // namespace arcxpress

#endif

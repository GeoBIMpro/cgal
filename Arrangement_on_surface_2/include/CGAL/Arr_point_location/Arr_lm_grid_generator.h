// Copyright (c) 2005  Tel-Aviv University (Israel).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you may redistribute it under
// the terms of the Q Public License version 1.0.
// See the file LICENSE.QPL distributed with CGAL.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
//
// Author(s)     : Idit Haran   <haranidi@post.tau.ac.il>
//                 Ron Wein     <wein@post.tau.ac.il>
#ifndef CGAL_ARR_LM_GRID_GENERATOR_H
#define CGAL_ARR_LM_GRID_GENERATOR_H

/*! \file
* Definition of the Arr_grid_landmarks_generator<Arrangement> template.
*/

#include <CGAL/Arr_observer.h>
#include <CGAL/Arrangement_2/Arr_traits_adaptor_2.h>
#include <CGAL/Arr_batched_point_location.h>

#include <list>
#include <algorithm>
#include <vector>

CGAL_BEGIN_NAMESPACE

/*! \class Arr_grid_landmarks_generator
 * A generator for the landmarks point-locatoion class, which uses a
 * set of points on a grid as its set of landmarks.
*/
template <class Arrangement_>
class Arr_grid_landmarks_generator :
    public Arr_observer <Arrangement_>
{
public:

  typedef Arrangement_                                      Arrangement_2;
  typedef Arr_grid_landmarks_generator<Arrangement_2>       Self;

  typedef typename Arrangement_2::Geometry_traits_2     Geometry_traits_2;
  typedef typename Arrangement_2::Vertex_const_iterator Vertex_const_iterator;
  typedef typename Arrangement_2::Vertex_const_handle   Vertex_const_handle;
  typedef typename Arrangement_2::Halfedge_const_handle Halfedge_const_handle;
  typedef typename Arrangement_2::Face_const_handle     Face_const_handle;
  typedef typename Arrangement_2::Vertex_handle         Vertex_handle;
  typedef typename Arrangement_2::Halfedge_handle       Halfedge_handle;
  typedef typename Arrangement_2::Face_handle           Face_handle;
  typedef typename Arrangement_2::Ccb_halfedge_circulator 
                                                      Ccb_halfedge_circulator;

  typedef typename Geometry_traits_2::Approximate_number_type    ANT;

  typedef typename Arrangement_2::Point_2                Point_2;

protected:

  typedef std::vector<Point_2>                           Points_set;
  typedef std::pair<Point_2,CGAL::Object>                PL_pair;
  typedef std::vector<PL_pair>                           Pairs_set;

  typedef Arr_traits_basic_adaptor_2<Geometry_traits_2>  Traits_adaptor_2;

  // Data members:
  const Traits_adaptor_2  *m_traits;
  bool                     ignore_notifications;
  bool                     updated;
  unsigned int             num_landmarks;
  Pairs_set                lm_pairs;

  ANT                      x_min, y_min;    // Bounding box for the
  ANT                      x_max, y_max;    // arrangement vertices.
  ANT                      step_x, step_y;  // Grid step sizes.
  unsigned int             sqrt_n;

private:

  /*! Copy constructor - not supported. */
  Arr_grid_landmarks_generator (const Self& );

  /*! Assignment operator - not supported. */
  Self& operator= (const Self& );

  
public: 

    /*! Constructor. */
  Arr_grid_landmarks_generator (const Arrangement_2& arr,
                                unsigned int n_landmarks = 0) :
    Arr_observer<Arrangement_2> (const_cast<Arrangement_2 &>(arr)),
    ignore_notifications (false),
    updated (false),
    num_landmarks (n_landmarks)
  {
    m_traits = static_cast<const Traits_adaptor_2*> (arr.geometry_traits());
    build_landmark_set();
  }
  
  /*!
   * Create the landmarks set (choosing the landmarks),
   * and store them in the nearest neighbor search structure.
   */
  virtual void build_landmark_set ()
  {
    // Create a set of points on a grid.
    Points_set    points; 
    
    _create_points_set(points);
    
    // Locate the landmarks in the arrangement using batched point-location
    // global function. Note that the resulting pairs are returned sorted by
    // their lexicographic xy-order.
    lm_pairs.clear();
    locate (*(this->arrangement()), points.begin(), points.end(),
            std::back_inserter(lm_pairs));

    updated = true;
    return;
  }

  /*!
   * Clear the set of landmarks.
   */
  virtual void clear_landmark_set ()
  {
    lm_pairs.clear();
    updated = false;

    return;
  }

  /*!
   * Get the nearest neighbor (landmark) to the given point.
   * \param q The query point.
   * \param obj Output: The location of the nearest landmark point in the
   *                    arrangement (a vertex, halfedge, or face handle).
   * \return The nearest landmark point.
   */
  virtual Point_2 closest_landmark (const Point_2& q, Object &obj)
  {
    CGAL_assertion(updated);

    // Calculate the index of the nearest grid point point to q.
    const ANT     qx = m_traits->approximate_2_object()(q, 0);
    const ANT     qy = m_traits->approximate_2_object()(q, 1);
    unsigned int  i, j;
    unsigned int  index;

    if (CGAL::compare (qx, x_min) == SMALLER)
      i = 0;
    else if (CGAL::compare (qx, x_max) == LARGER)
      i = sqrt_n - 1;
    else 
      i = static_cast<int>(((qx - x_min) / step_x) + 0.5);

    if (CGAL::compare (qy, y_min) == SMALLER)
      j = 0;
    else if (CGAL::compare (qy, y_max) == LARGER)
      j = sqrt_n - 1;
    else 
      j = static_cast<int>(((qy - y_min) / step_y) + 0.5);

    index = sqrt_n * i + j;

    // Return the result.
    obj = lm_pairs[index].second;
    return (lm_pairs[index].first);
  }

  /// \name Overloaded observer functions on global changes.
  //@{

  /*! 
   * Notification before the arrangement is assigned with another
   * arrangement.
   * \param arr The arrangement to be copied.
   */
  virtual void before_assign (const Arrangement_2& arr)
  {
    clear_landmark_set();
    m_traits = static_cast<const Traits_adaptor_2*> (arr.geometry_traits());
    ignore_notifications = true;
  }

  /*!
   * Notification after the arrangement has been assigned with another
   * arrangement.
   */
  virtual void after_assign ()
  { 
    build_landmark_set();
    ignore_notifications = false;
  }

  /*! 
   * Notification before the observer is attached to an arrangement.
   * \param arr The arrangement we are about to attach the observer to.
   */
  virtual void before_attach (const Arrangement_2& arr)
  {
    clear_landmark_set();
    m_traits = static_cast<const Traits_adaptor_2*> (arr.geometry_traits());
    ignore_notifications = true;
  }

  /*!
   * Notification after the observer has been attached to an arrangement.
   */
  virtual void after_attach ()
  {
    build_landmark_set();
    ignore_notifications = false;
  }

  /*! 
   * Notification before the observer is detached from the arrangement.
   */
  virtual void before_detach ()
  {
    clear_landmark_set();
  }

  /*!
   * Notification after the arrangement is cleared.
   * \param u A handle to the unbounded face.
   */
  virtual void after_clear ()
  { 
    clear_landmark_set();
    build_landmark_set();
  }

  /*! Notification before a global operation modifies the arrangement. */
  virtual void before_global_change ()
  {
    clear_landmark_set();
    ignore_notifications = true;
  }

  /*! Notification after a global operation is completed. */
  virtual void after_global_change ()
  {
    build_landmark_set();
    ignore_notifications = false;
  }
  //@}

  /// \name Overloaded observer functions on local changes.
  //@{

  /*! Notification after the creation of a new vertex. */
  virtual void after_create_vertex (Vertex_handle )
  {
    _handle_local_change_notification();
  }

  /*! Notification after the creation of a new edge. */
  virtual void after_create_edge (Halfedge_handle )
  {
    _handle_local_change_notification();
  }

  /*! Notification after an edge was split. */
  virtual void after_split_edge (Halfedge_handle ,
                                 Halfedge_handle )
  {
    _handle_local_change_notification();
  }

  /*! Notification after a face was split. */
  virtual void after_split_face (Face_handle ,
                                 Face_handle ,
                                 bool )
  {
    _handle_local_change_notification();
  }

  /*! Notification after an outer CCB was split.*/
  virtual void after_split_outer_ccb (Face_handle ,
                                      Ccb_halfedge_circulator ,
                                      Ccb_halfedge_circulator )
  {
    _handle_local_change_notification();
  }

  /*! Notification after an inner CCB was split. */
  virtual void after_split_inner_ccb (Face_handle ,
                                      Ccb_halfedge_circulator ,
                                      Ccb_halfedge_circulator )
  {
    _handle_local_change_notification();
  }

  /*! Notification after an outer CCB was added to a face. */
  virtual void after_add_outer_ccb (Ccb_halfedge_circulator )
  {
    _handle_local_change_notification();
  }

  /*! Notification after an inner CCB was created inside a face. */
  virtual void after_add_inner_ccb (Ccb_halfedge_circulator )
  {
    _handle_local_change_notification();
  }

  /*! Notification after an isolated vertex was created inside a face. */
  virtual void after_add_isolated_vertex (Vertex_handle )
  {
    _handle_local_change_notification();
  }

  /*! Notification after an edge was merged. */
  virtual void after_merge_edge (Halfedge_handle )
  {
    _handle_local_change_notification();
  }

  /*! Notification after a face was merged. */
  virtual void after_merge_face (Face_handle )
  {
    _handle_local_change_notification();
  }

  /*! Notification after an outer CCB was merged. */
  virtual void after_merge_outer_ccb (Face_handle ,
                                      Ccb_halfedge_circulator )
  {
    _handle_local_change_notification();
  }

  /*! Notification after an inner CCB was merged. */
  virtual void after_merge_inner_ccb (Face_handle ,
                                      Ccb_halfedge_circulator )
  {
    _handle_local_change_notification();
  }

  /*! Notification after an outer CCB is moved from one face to another. */
  virtual void after_move_outer_ccb (Ccb_halfedge_circulator )
  {
    _handle_local_change_notification();
  }

  /*! Notification after an inner CCB is moved from one face to another. */
  virtual void after_move_inner_ccb (Ccb_halfedge_circulator )
  {
    _handle_local_change_notification();
  }

  /*! Notification after an isolated vertex is moved. */
  virtual void after_move_isolated_vertex (Vertex_handle )
  {
    _handle_local_change_notification();
  }

  /*! Notificaion after the removal of a vertex. */
  virtual void after_remove_vertex ()
  {
    _handle_local_change_notification();
  }

  /*! Notification after the removal of an edge. */
  virtual void after_remove_edge ()
  {
    _handle_local_change_notification();
  }

  /*! Notificaion after the removal of an outer CCB. */
  virtual void after_remove_outer_ccb (Face_handle )
  {
    _handle_local_change_notification();
  }

  /*! Notificaion after the removal of an inner CCB. */
  virtual void after_remove_inner_ccb (Face_handle )
  {
    _handle_local_change_notification();
  }
  //@}

protected:

  /*! Handle a change notification. */
  void _handle_local_change_notification ()
  {
    if (! ignore_notifications)
    {
      clear_landmark_set();
      build_landmark_set();
    }
    return;
  }

  /*!
   * Create a set of landmark points on a grid.
   */
  virtual void _create_points_set (Points_set & points)
  {
    Arrangement_2 *arr = this->arrangement();

    if(arr->is_empty())
      return;

    // Locate the arrangement vertices with minimal and maximal x and
    // y-coordinates.
    Vertex_const_iterator    vit = arr->vertices_begin();
    x_min = x_max = m_traits->approximate_2_object()(vit->point(), 0);
    y_min = y_max = m_traits->approximate_2_object()(vit->point(), 1);

    if(arr->number_of_vertices() == 1)
    {
      // There is only one isolated vertex at the arrangement:
      step_x = step_y = 1;
      sqrt_n = 1;
      points.push_back (Point_2 (x_min, y_min));
      return;
    }

    ANT                      x, y;
    Vertex_const_iterator    left, right, top, bottom;
    
    left = right = top = bottom = vit;

    for (++vit; vit != arr->vertices_end(); ++vit)
    {
      x = m_traits->approximate_2_object()(vit->point(), 0);
      y = m_traits->approximate_2_object()(vit->point(), 1);

      if (CGAL::compare (x, x_min) == SMALLER)
      {
        x_min = x;
        left = vit;
      }
      else if (CGAL::compare (x, x_max) == LARGER)
      {
        x_max = x;
        right = vit;
      }

      if (CGAL::compare (y, y_min) == SMALLER)
      {
        y_min = y;
        bottom = vit;
      }
      else if (CGAL::compare (y, y_max) == LARGER)
      {
        y_max = y;
        top = vit;
      }
    }

    // Create N Halton points. If N was not given to the constructor,
    // set it to be the number of vertices V in the arrangement (actually
    // we generate ceiling(sqrt(V))^2 landmarks to obtain a square grid).
    if (num_landmarks == 0)
      num_landmarks = arr->number_of_vertices();

    sqrt_n = static_cast<unsigned int>
      (std::sqrt(static_cast<double> (num_landmarks)) + 0.99999);
    num_landmarks = sqrt_n * sqrt_n;

    CGAL_assertion (sqrt_n > 1);

    // Calculate the step sizes for the grid.
    ANT    delta_x = m_traits->approximate_2_object()(right->point(), 0) -
                     m_traits->approximate_2_object()(left->point(), 0);
    ANT    delta_y = m_traits->approximate_2_object()(top->point(), 1) -
                     m_traits->approximate_2_object()(bottom->point(), 1);

    if (CGAL::sign (delta_x) == CGAL::ZERO)
      delta_x = delta_y;

    if (CGAL::sign (delta_y) == CGAL::ZERO)
      delta_y = delta_x;

    CGAL_assertion (CGAL::sign (delta_x) == CGAL::POSITIVE &&
                    CGAL::sign (delta_y) == CGAL::POSITIVE);

    step_x = delta_x / (sqrt_n - 1);
    step_y = delta_y / (sqrt_n - 1);

    // Create the points on the grid.
    const double  x_min =
      CGAL::to_double (m_traits->approximate_2_object()(left->point(), 0));
    const double  y_min =
      CGAL::to_double (m_traits->approximate_2_object()(bottom->point(), 1));
    const double  sx = CGAL::to_double (step_x);
    const double  sy = CGAL::to_double (step_y);
    double        px, py;
    unsigned int  i, j;

    for (i = 0; i< sqrt_n; i++)
    {
      px = x_min + i*sx;

      for (j = 0; j< sqrt_n; j++)
      {
        py = y_min + j*sy;

        points.push_back (Point_2 (px, py)); 

      }
    }

    return;
  }

};

CGAL_END_NAMESPACE

#endif

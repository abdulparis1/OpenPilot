/**
******************************************************************************
*
* @file       pureprojection.cpp
* @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
*             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
* @brief      
* @see        The GNU Public License (GPL) Version 3
* @defgroup   OPMapWidget
* @{
* 
*****************************************************************************/
/* 
* This program is free software; you can redistribute it and/or modify 
* it under the terms of the GNU General Public License as published by 
* the Free Software Foundation; either version 3 of the License, or 
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful, but 
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
* for more details.
* 
* You should have received a copy of the GNU General Public License along 
* with this program; if not, write to the Free Software Foundation, Inc., 
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#include "pureprojection.h"





 
namespace internals {
Point PureProjection::FromLatLngToPixel(const PointLatLng::PointLatLng &p,const int &zoom)
      {
         return FromLatLngToPixel(p.Lat(), p.Lng(), zoom);
      }


     PointLatLng PureProjection::FromPixelToLatLng(const Point &p,const int &zoom)
      {
         return FromPixelToLatLng(p.X(), p.Y(), zoom);
      }

      Point PureProjection::FromPixelToTileXY(const Point &p)
      {
         return Point((int) (p.X() / TileSize().Width()), (int) (p.Y() / TileSize().Height()));
      }

      Point PureProjection::FromTileXYToPixel(const Point &p)
      {
         return Point((p.X() * TileSize().Width()), (p.Y() * TileSize().Height()));
      }

      Size PureProjection::GetTileMatrixSizeXY(const int &zoom)
      {
         Size sMin = GetTileMatrixMinXY(zoom);
         Size sMax = GetTileMatrixMaxXY(zoom);

         return  Size(sMax.Width() - sMin.Width() + 1, sMax.Height() - sMin.Height() + 1);
      }
      int PureProjection::GetTileMatrixItemCount(const int &zoom)
      {
         Size s = GetTileMatrixSizeXY(zoom);
         return (s.Width() * s.Height());
      }
      Size PureProjection::GetTileMatrixSizePixel(const int &zoom)
      {
         Size s = GetTileMatrixSizeXY(zoom);
         return Size(s.Width() * TileSize().Width(), s.Height() * TileSize().Height());
      }
      QList<Point> PureProjection::GetAreaTileList(const RectLatLng &rect,const int &zoom,const int &padding)
      {
         QList<Point> ret;

         Point topLeft = FromPixelToTileXY(FromLatLngToPixel(rect.LocationTopLeft(), zoom));
         Point rightBottom = FromPixelToTileXY(FromLatLngToPixel(rect.Bottom(), rect.Right(), zoom));

         for(int x = (topLeft.X() - padding); x <= (rightBottom.X() + padding); x++)
         {
            for(int y = (topLeft.Y() - padding); y <= (rightBottom.Y() + padding); y++)
            {
               Point p = Point(x, y);
               if(!ret.contains(p) && p.X() >= 0 && p.Y() >= 0)
               {
                  ret.append(p);
               }
            }
         }
         //ret.TrimExcess();

         return ret;
      }
      double PureProjection::GetGroundResolution(const int &zoom,const double &latitude)
      {
          return (cos(latitude * (PI / 180)) * 2 * PI * Axis()) / GetTileMatrixSizePixel(zoom).Width();
      }

      double PureProjection::Sign(const double &x)
      {
         if(x < 0.0)
            return (-1);
         else
            return (1);
      }

      double PureProjection::AdjustLongitude(double x)
      {
         qlonglong count = 0;
         while(true)
         {
            if(qAbs(x) <= PI)
               break;
            else
               if(((qlonglong) qAbs(x / PI)) < 2)
                  x = x - (Sign(x) * TWO_PI);

               else
                  if(((qlonglong) qAbs(x / TWO_PI)) < MAXLONG)
                  {
                     x = x - (((qlonglong) (x / TWO_PI)) * TWO_PI);
                  }
                  else
                     if(((qlonglong) qAbs(x / (MAXLONG * TWO_PI))) < MAXLONG)
                     {
                        x = x - (((qlonglong) (x / (MAXLONG * TWO_PI))) * (TWO_PI * MAXLONG));
                     }
                     else
                        if(((qlonglong) qAbs(x / (DBLLONG * TWO_PI))) < MAXLONG)
                        {
                           x = x - (((qlonglong) (x / (DBLLONG * TWO_PI))) * (TWO_PI * DBLLONG));
                        }
                        else
                           x = x - (Sign(x) * TWO_PI);
            count++;
            if(count > MAX_VAL)
               break;
         }
         return (x);
      }

      void PureProjection::SinCos(const double &val,  double &si, double &co)
      {
         si = sin(val);
         co = cos(val);
      }

      double PureProjection::e0fn(const double &x)
      {
         return (1.0 - 0.25 * x * (1.0 + x / 16.0 * (3.0 + 1.25 * x)));
      }

       double PureProjection::e1fn(const double &x)
      {
         return (0.375 * x * (1.0 + 0.25 * x * (1.0 + 0.46875 * x)));
      }

       double PureProjection::e2fn(const double &x)
      {
         return (0.05859375 * x * x * (1.0 + 0.75 * x));
      }

       double PureProjection::e3fn(const double &x)
      {
         return (x * x * x * (35.0 / 3072.0));
      }

       double PureProjection::mlfn(const double &e0,const double &e1,const double &e2,const double &e3,const double &phi)
      {
         return (e0 * phi - e1 * sin(2.0 * phi) + e2 * sin(4.0 * phi) - e3 * sin(6.0 * phi));
      }

       qlonglong PureProjection::GetUTMzone(const double &lon)
      {
         return ((qlonglong) (((lon + 180.0) / 6.0) + 1.0));
      }


      void PureProjection::FromGeodeticToCartesian(double Lat,double Lng,const double &Height,  double &X,  double &Y,  double &Z)
      {
         Lat = (PI / 180) * Lat;
         Lng = (PI / 180) * Lng;

         double B = Axis() * (1.0 - Flattening());
         double ee = 1.0 - (B / Axis()) * (B / Axis());
         double N = (Axis() / sqrt(1.0 - ee * sin(Lat) * sin(Lat)));

         X = (N + Height) * cos(Lat) * cos(Lng);
         Y = (N + Height) * cos(Lat) * sin(Lng);
         Z = (N * (B / Axis()) * (B / Axis()) + Height) * sin(Lat);
      }
    void PureProjection::FromCartesianTGeodetic(const double &X,const double &Y,const double &Z,  double &Lat,  double &Lng)
      {
         double E = Flattening() * (2.0 - Flattening());
         Lng = atan2(Y, X);

         double P = sqrt(X * X + Y * Y);
         double Theta = atan2(Z, (P * (1.0 - Flattening())));
         double st = sin(Theta);
         double ct = cos(Theta);
         Lat = atan2(Z + E / (1.0 - Flattening()) * Axis() * st * st * st, P - E * Axis() * ct * ct * ct);

         Lat /= (PI / 180);
         Lng /= (PI / 180);
      }
}
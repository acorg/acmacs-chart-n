#pragma once

#include "acmacs-base/layout.hh"

// ----------------------------------------------------------------------

namespace acmacs
{
    class BoundingBall
    {
     public:
        BoundingBall() = default;
          // aP1 and aP2 are opposite corners of some area, draw a minimal circle through them
        BoundingBall(const PointCoordinates& aP1, const PointCoordinates& aP2)
            : mCenter(acmacs::middle(aP1, aP2))
            {
                PointCoordinates v{aP1};
                v -= aP2;
                mDiameter = std::sqrt(std::inner_product(v.begin(), v.end(), v.begin(), 0.0));
                mHVDiameter = calculate_hv_diameter(v);
            }

        const PointCoordinates& center() const { return mCenter; }
        double diameter() const { return mDiameter; }
        double hv_diameter() const { return mHVDiameter; }
        double hv_diameter_whole_number() const { return std::ceil(mHVDiameter); }

        void set(const PointCoordinates& aCenter, double aDiameter)
            {
                mCenter = aCenter;
                mDiameter = aDiameter;
                mHVDiameter = mDiameter;
            }

        void move(const PointCoordinates& aP1, const PointCoordinates& aP2, double aDistance)
            {
                mDiameter = aDistance;
                mHVDiameter = mDiameter;
                mCenter = acmacs::middle(aP1, aP2);
            }

        void moveCenter(const PointCoordinates& aBy) { mCenter += aBy; }

          // Extends bounding ball (change center and diameter) to make sure the bounding ball includes the passed point
        void extend(const PointCoordinates& aPoint);

          // Extends bounding ball to make sure it includes all points of the second boundig ball
        void extend(const BoundingBall& aBoundingBall);

        double top() const { return mCenter.y() - mDiameter / 2.0; }
        double bottom() const { return mCenter.y() + mDiameter / 2.0; }
        double left() const { return mCenter.x() - mDiameter / 2.0; }
        double right() const { return mCenter.x() + mDiameter / 2.0; }

     private:
        PointCoordinates mCenter{0.0, 0.0};
        double mDiameter{0};
        double mHVDiameter{0};  // to calculate smaller map size

        // Returns distance^2 from the ball center to point
        double distance2FromCenter(const PointCoordinates& aPoint) const { return distance2(aPoint, mCenter); }

        double radius2() const { return mDiameter * mDiameter * 0.25; }

        double calculate_hv_diameter(const PointCoordinates& dist) const;

    }; // class BoundingBall

    BoundingBall minimum_bounding_ball(const Layout& aLayout);

} // namespace acmacs

// ----------------------------------------------------------------------

// inline std::ostream& operator<<(std::ostream& out, const acmacs::BoundingBall& a)
// {
//     return out << "BoundingBall(" << a.center() << ", " << a.diameter() << ")";
// }

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:

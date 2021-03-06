/******************************************************************************
*       SOFA, Simulation Open-Framework Architecture, development version     *
*                (c) 2006-2017 INRIA, USTL, UJF, CNRS, MGH                    *
*                                                                             *
* This program is free software; you can redistribute it and/or modify it     *
* under the terms of the GNU Lesser General Public License as published by    *
* the Free Software Foundation; either version 2.1 of the License, or (at     *
* your option) any later version.                                             *
*                                                                             *
* This program is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License *
* for more details.                                                           *
*                                                                             *
* You should have received a copy of the GNU Lesser General Public License    *
* along with this program. If not, see <http://www.gnu.org/licenses/>.        *
*******************************************************************************
* Authors: The SOFA Team and external contributors (see Authors.txt)          *
*                                                                             *
* Contact information: contact@sofa-framework.org                             *
******************************************************************************/

#ifndef FLEXIBLE_MooneyRivlinMaterialBlock_H
#define FLEXIBLE_MooneyRivlinMaterialBlock_H


#include "../material/BaseMaterial.h"
#include "../BaseJacobian.h"
#include <sofa/defaulttype/Vec.h>
#include <sofa/defaulttype/Mat.h>
#include "../types/StrainTypes.h"
#include <sofa/helper/decompose.h>

namespace sofa
{

namespace defaulttype
{


//////////////////////////////////////////////////////////////////////////////////
////  default implementation for U331
//////////////////////////////////////////////////////////////////////////////////

template<class _T>
class MooneyRivlinMaterialBlock:
    public  BaseMaterialBlock< _T >
{
public:
    typedef _T T;

    typedef BaseMaterialBlock<T> Inherit;
    typedef typename Inherit::Coord Coord;
    typedef typename Inherit::Deriv Deriv;
    typedef typename Inherit::MatBlock MatBlock;
    typedef typename Inherit::Real Real;

    /**
      * DOFs: principal stretches U1,U2,U3   J=U1*U2*U3
      *
      * classic Mooney rivlin
      *     - W = vol * [ C1 ( (U1^2+U2^2+U3^2)/J^{2/3} - 3)  + C2 ( (U1^2.U2^2 + U2^2.U3^2 + U3^2.U1^2))/J^{4/3} - 3) + bulk/2 (J-1)^2]
      * see maple file ./doc/mooneyRivlin_principalStretches.mw for derivative
      */

    static const bool constantK=false;

    Real C1Vol;  ///<  first coef * volume
    Real C2Vol;  ///<  second coef * volume
    Real bulkVol;   ///<  volume coef * volume
    bool stabilization;

    mutable MatBlock _K;

    void init(const Real &C1,const Real &C2,const Real &bulk, bool _stabilization)
    {
        Real vol=1.;
        if(this->volume) vol=(*this->volume)[0];
        C1Vol = C1*vol;
        C2Vol = C2*vol;
        bulkVol = bulk*vol;
        stabilization = _stabilization;
    }

    Real getPotentialEnergy(const Coord& x) const
    {
        const Real& U1 = x.getStrain()[0];
        const Real& U2 = x.getStrain()[1];
        const Real& U3 = x.getStrain()[2];

        Real J = U1*U2*U3;
        Real Jm1 = J-1;
        Real squareU[3] = { U1*U1, U2*U2, U3*U3 };
        return C1Vol*((squareU[0]+squareU[1]+squareU[2])*pow(J,static_cast<Real>(-2.0/3.0))-(Real)3.) +
                C2Vol*((squareU[0]*squareU[1]+squareU[1]*squareU[2]+squareU[2]*squareU[0])*pow(J,static_cast<Real>(-4.0/3.0))-(Real)3.) +
                0.5*bulkVol*Jm1*Jm1;
    }

    void addForce( Deriv& f, const Coord& x, const Deriv& /*v*/) const
    {
        const Real& U1 = x.getStrain()[0];
        const Real& U2 = x.getStrain()[1];
        const Real& U3 = x.getStrain()[2];

        // TODO optimize this crappy code generated by maple
        // there are a lot of redondencies between computation of f and K

        Real t1 =  U1 *  U2;

        const Real J = t1 * U3;

        const Real Jm1 = J-1;

        const Real Jm23 = pow(J,-2.0/3.0);
        const Real Jm43 = pow(J,-4.0/3.0);
        const Real Jm53 = pow(J,-5.0/3.0);
        const Real Jm73 = pow(J,-7.0/3.0);
        const Real Jm83 = pow(J,-8.0/3.0);
        const Real Jm103 = pow(J,-10.0/3.0);

        Real t3 = Jm23;
        Real t4 =  U3 *  U3;
        Real t5 =  U2 *  U2;
        Real t6 =  U1 *  U1;
        Real t7 = -0.2e1 / 0.3e1;
        Real t8 = 2 * U1;
        t7 = t7 * (t6 + t5 + t4) * Jm53;
        Real t9 = Jm43;
        Real t10 = t6 + t5;
        Real t11 = -0.4e1 / 0.3e1;
        t11 = t11 * (t10 * t4 + t6 * t5) * Jm73;
        Real t2 = bulkVol * Jm1;
        Real t12 = 2 * U2;
        Real t13 = 2 * U3;

        f.getStrain()[0] -= C1Vol * ( t8 * t3 + t7 *  U2 *  U3) + C2Vol * ( t8 * (t5 + t4) * t9 + t11 *  U2 *  U3) + t2 *  U2 *  U3;
        f.getStrain()[1] -= C1Vol * ( t12 * t3 + t7 *  U1 *  U3) + C2Vol * ( t12 * (t6 + t4) * t9 + t11 *  U1 *  U3) + t2 *  U1 *  U3;
        f.getStrain()[2] -= C1Vol * ( t13 * t3 + t7 * t1) + C2Vol * ( t13 * t10 * t9 + t11 * t1) + t2 * t1;

        t7 = t6 + t5 + t4;
        t8 = Jm83;
        t9 =  U2 * U3;
        t10 = 0.2e1 * Jm23;
        t11 = t5 + t4;
        t12 = 2 * t11;
        t13 = Jm43;
        Real t14 = t12 * U1;
        Real t15 = Jm73;
        Real t16 = t6 + t5;
        Real t17 = t16 * t4 + t6 * t5;
        Real t18 = Jm103;
        Real t19 = bulkVol * t5;
        Real t20 = (0.10e2 / 0.9e1 * J * t8 - 0.2e1 / 0.3e1 * Jm53) * t7;
        Real t21 = t6 + t4;
        Real t22 = 2 * t21;
        Real t23 = t22 * U2;
        Real t24 = 0.4e1 * t13;
        Real t25 = 0.28e2 / 0.9e1 * t17 * t18;
        Real t26 = t14 * U1;
        Real t27 = t23 * U2;
        t2 = 0.2e1 * Jm1;
        Real t28 = bulkVol * U3 * t2 + C1Vol * U3 * (t20 - 0.4e1 / 0.3e1 * Jm53 * t16) + C2Vol * (-0.4e1 / 0.3e1 * ( t26 +  t27 + t17) * U3 * t15 + t1 * (t24 + t25 * t4));
        t16 = 0.2e1 * t16;
        Real t29 = t16 * U3;
        Real t30 = t29 * U3;
        Real t31 =  U1 * U3;
        t5 = bulkVol *  U2 * t2 + C1Vol *  U2 * (t20 - 0.4e1 / 0.3e1 * Jm53 *  t21) + C2Vol * (-0.4e1 / 0.3e1 * ( t26 + t30 + t17) *  U2 * t15 + t31 * (t24 + t25 * t5));
        t21 = -(0.8e1 / 0.3e1 * Jm53);
        t2 = bulkVol *  U1 * t2 + C1Vol *  U1 * (t20 - 0.4e1 / 0.3e1 * Jm53 *  t11) + C2Vol* (-0.4e1 / 0.3e1 * ( t27 + t30 + t17) *  U1 * t15 + t9 * (t24 + t25 * t6));
        _K[0][0] = ( C1Vol * (t9 * (-0.8e1 / 0.3e1 *  U1 * Jm53 + 0.10e2 / 0.9e1 * t9 * t7 * t8) + t10) + C2Vol * (t9 * (-0.8e1 / 0.3e1 *  t14 * t15 + 0.28e2 / 0.9e1 * t9 * t17 * t18) +  t12 * t13) + t19 * t4 );
        _K[0][1] = t28;
        _K[0][2] = t5;
        _K[1][0] = _K[0][1];
        _K[1][1] = ( C1Vol * (t31 * ( (t21 * U2) + 0.10e2 / 0.9e1 * t31 * t7 * t8) + t10) + C2Vol * (t31 * (-0.8e1 / 0.3e1 *  t23 * t15 + t31 * t25) +  t22 * t13) + bulkVol * t6 * t4 );
        _K[1][2] = t2;
        _K[2][0] = _K[0][2];
        _K[2][1] = _K[1][2];
        _K[2][2] = ( C1Vol * (t1 * ( t21 * U3 + 0.10e2 / 0.9e1 * t1 * t7 * t8) + t10) + C2Vol * (t1 * (-0.8e1 / 0.3e1 * t29 * t15 + t25 * t1) + t16 * t13) + t19 * t6 );

        // ensure _K is symmetric positive semi-definite (even if it is not as good as positive definite) as suggested in [Teran05]
        if( stabilization ) helper::Decompose<Real>::PSDProjection( _K );
    }

    void addDForce( Deriv& df, const Deriv& dx, const SReal& kfactor, const SReal& /*bfactor*/ ) const
    {
        df.getStrain() -= _K * dx.getStrain() * kfactor;
    }

    MatBlock getK() const
    {
        return -_K;
    }

    MatBlock getC() const
    {
        MatBlock C = MatBlock();
        C.invert( _K );
        return C;
    }

    MatBlock getB() const
    {
        return MatBlock();
    }
};




//////////////////////////////////////////////////////////////////////////////////
////  U321
//////////////////////////////////////////////////////////////////////////////////

template<class _Real>
class MooneyRivlinMaterialBlock< U321(_Real) >:
    public  BaseMaterialBlock< U321(_Real) >
{
public:
    typedef U321(_Real) T;

    typedef BaseMaterialBlock<T> Inherit;
    typedef typename Inherit::Coord Coord;
    typedef typename Inherit::Deriv Deriv;
    typedef typename Inherit::MatBlock MatBlock;
    typedef typename Inherit::Real Real;

    /**
      * DOFs: principal stretches U1,U2   J=U1*U2
      *
      * classic Mooney rivlin
      *     - W = vol * [ C1 ( (U1^2+U2^2+1)/J^{2/3} - 3)  + C2 ( (U1^2.U2^2 + U2^2 + U1^2))/J^{4/3} - 3) + bulk/2 (J-1)^2]
      * see maple file ./doc/mooneyRivlin_principalStretches.mw for derivative
      */

    static const bool constantK=false;

    Real C1Vol;  ///<  first coef * volume
    Real C2Vol;  ///<  second coef * volume
    Real bulkVol;   ///<  volume coef * volume
    bool stabilization;

    mutable MatBlock _K;

    void init(const Real &C1,const Real &C2,const Real &bulk, bool _stabilization)
    {
        Real vol=1.;
        if(this->volume) vol=(*this->volume)[0];
        C1Vol = C1*vol;
        C2Vol = C2*vol;
        bulkVol = bulk*vol;
        stabilization = _stabilization;
    }

    Real getPotentialEnergy(const Coord& x) const
    {
        const Real& U1 = x.getStrain()[0];
        const Real& U2 = x.getStrain()[1];

        Real J = U1*U2;
        Real Jm1 = J-1;
        Real squareU[3] = { U1*U1, U2*U2 };
        return C1Vol*((squareU[0]+squareU[1]+1)*pow(J,-2.0/3.0)-(Real)3.) +
                C2Vol*((squareU[0]*squareU[1]+squareU[1]+squareU[0])*pow(J,-4.0/3.0)-(Real)3.) +
                0.5*bulkVol*Jm1*Jm1;
    }

    void addForce( Deriv& f, const Coord& x, const Deriv& /*v*/) const
    {
        const Real& U1 = x.getStrain()[0];
        const Real& U2 = x.getStrain()[1];

        Real squareU[2] = { U1*U1, U2*U2 };


        const Real J =  U1 *  U2;
        const Real Jm1 = J-1;

        const Real Jm23 = pow(J,-2.0/3.0);
        const Real Jm43 = pow(J,-4.0/3.0);
        const Real Jm53 = pow(J,-5.0/3.0);
        const Real Jm73 = pow(J,-7.0/3.0);
        const Real Jm83 = pow(J,-8.0/3.0);
        const Real Jm103 = pow(J,-10.0/3.0);

        const Real biU1 = 2 * U1;
        const Real biU2 = 2 * U2;


        const Real I1 = squareU[0] + squareU[1] + 1;

        Real t5 = -2.0/3.0 * I1 * Jm53;
        Real t3 = -4.0/3.0 * ( squareU[0]*squareU[1] + squareU[0] + squareU[1] ) * Jm73;
        Real t1 = bulkVol * Jm1;

        f.getStrain()[0] -= C1Vol * ( biU1 * Jm23 + t5 *  U2) + C2Vol * ( biU1 * (squareU[1] + 1) * Jm43 +  (t3 * U2) ) + t1 *  U2;
        f.getStrain()[1] -= C1Vol * ( biU2 * Jm23 + t5 *  U1) + C2Vol * ( biU2 * (squareU[0] + 1) * Jm43 +  (t3 * U1) ) + t1 *  U1;



        // TODO optimize this crappy code generated by maple

        Real t7 = 0.10e2 / 0.9e1 *  I1 * Jm83;
        Real t8 = 2.0 * Jm23;
        Real t10 = biU1 * (squareU[1] + 1);
        Real t12 = squareU[0] + 1;
        Real t13 = t12 * squareU[1] + squareU[0];
        Real t14 = 0.28e2 / 0.9e1 *  t13 * Jm103;
        t12 = biU2 * t12;
        _K[0][0] = C1Vol * (t8 + (-0.8e1 / 0.3e1 *  U1 * Jm53 + t7 *  U2) *  U2) + C2Vol * ( (2 * squareU[1] + 2) * Jm43 + (-0.8e1 / 0.3e1 *  t10 * Jm73 + t14 *  U2) *  U2) + bulkVol *  squareU[1];
        _K[0][1] = (0.2e1 * Jm1) * bulkVol + C1Vol * ((0.10e2 / 0.9e1 * J * Jm83 - 0.2e1 / 0.3e1 * Jm53) *  I1 - 0.4e1 / 0.3e1 * Jm53 *  (squareU[0] + squareU[1])) + C2Vol * ((-0.4e1 / 0.3e1 *  t10 *  U1 - 0.4e1 / 0.3e1 *  U2 *  t12 - 0.4e1 / 0.3e1 *  t13) * Jm73 + J * (0.4e1 * Jm43 + t14));
        _K[1][0] = _K[0][1];
        _K[1][1] = C1Vol * (t8 + (-0.8e1 / 0.3e1 * Jm53 *  U2 + t7 *  U1) *  U1) + C2Vol * ( (2 * squareU[0] + 2) * Jm43 + (-0.8e1 / 0.3e1 *  t12 * Jm73 + t14 *  U1) *  U1) + bulkVol *  squareU[0];


        // ensure _K is symmetric positive semi-definite (even if it is not as good as positive definite) as suggested in [Teran05]
        if( stabilization ) helper::Decompose<Real>::PSDProjection( _K );
    }

    void addDForce( Deriv& df, const Deriv& dx, const SReal& kfactor, const SReal& /*bfactor*/ ) const
    {
        df.getStrain() -= _K * dx.getStrain() * kfactor;
    }

    MatBlock getK() const
    {
        return -_K;
    }

    MatBlock getC() const
    {
        MatBlock C = MatBlock();
        C.invert( _K );
        return C;
    }

    MatBlock getB() const
    {
        return MatBlock();
    }
};


//////////////////////////////////////////////////////////////////////////////////
////  I331
//////////////////////////////////////////////////////////////////////////////////

//    - deviatoric \f$ F -> [ I1/J^{2/3} , I2/J^{4/3}, J ] \f$
//    - deviatoric \f$ J = [ d(sqrt(I1))/J^{1/3} - dJ*sqrt(I1/J^{2/3})/3J, d(sqrt(I2))/J^{2/3} - dJ*sqrt(I2/J^{2/3})*2/3J, dJ ] \f$

//* The energy is : C1 ( I1/ J^2/3  - 3)  + C2 ( I2/ J^4/3  - 3) + bulk/2 (J-1)^2


template<class _Real>
class MooneyRivlinMaterialBlock< I331(_Real) > :
    public  BaseMaterialBlock< I331(_Real) >
{
public:
    typedef I331(_Real) T;

    typedef BaseMaterialBlock<T> Inherit;
    typedef typename Inherit::Coord Coord;
    typedef typename Inherit::Deriv Deriv;
    typedef typename Inherit::MatBlock MatBlock;
    typedef typename Inherit::Real Real;

    /**
      * DOFs: I1, I2, J
      *
      * classic Mooney rivlin
      *     - W = vol* [ C1 ( I1/J^2/3 - 3)  + C2 ( I2/J^4/3 - 3) + bulk/2 (J -1)^2 ]
      *     - f = vol [ -C1/J^2/3 , -C2/J^4/3 , (2/3)*C1*I1/J^(5/3) + (4/3)*C2*I2/J^(7/3) - bulk*(J-1) ]
      *     - K  =   vol [ 0                , 0                 , (2/3)*C1/J^(5/3)                                 ]
      *                  [ 0                , 0                 , (4/3)*C2/J^(7/3)                                 ]
      *                  [ (2/3)*C1/J^(5/3) , (4/3)*C2/J^(7/3)  , -(10/9)*C1*I1/J^(8/3)-(28/9)*C2*I2/J^(10/3)-bulk ]
      */

    static const bool constantK=false;

    Real C1Vol;  ///<  first coef * volume
    Real C2Vol;  ///<  second coef * volume
    Real bulkVol; ///< bulk modulus * volume
    mutable Real K02;
    mutable Real K12;
    mutable Real K22;

    void init(const Real &C1,const Real &C2,const Real &bulk,bool)
    {
        Real vol=1.;
        if(this->volume) vol=(*this->volume)[0];
        C1Vol = C1 * vol;
        C2Vol = C2 * vol;
        bulkVol = bulk * vol;
    }

    Real getPotentialEnergy(const Coord& x) const
    {
        Real Jm23=pow(x.getStrain()[2],-(Real)2./(Real)3.);
        Real Jm43=Jm23*Jm23;

        return C1Vol*(x.getStrain()[0]*Jm23-(Real)3.) +
               C2Vol*(x.getStrain()[1]*Jm43-(Real)3.) +
               bulkVol*(Real)0.5*(x.getStrain()[2]-1.0)*(x.getStrain()[2]-1.0);
    }

    void addForce( Deriv& f , const Coord& x , const Deriv& /*v*/) const
    {
        Real Jm13=pow(x.getStrain()[2],-(Real)1./(Real)3.);
        Real Jm23=Jm13*Jm13;
        Real Jm43=Jm23*Jm23;
        Real Jm53=Jm43*Jm13;
        Real Jm73=Jm53*Jm23;

        K02 = 2./3.*C1Vol*Jm53;
        K12 = 4./3.*C2Vol*Jm73;
        K22 = -bulkVol;
        K22 -= (10./9.)*C1Vol*x.getStrain()[0]*Jm73*Jm13;
        K22 -= (28./9.)*C2Vol*x.getStrain()[0]*Jm53*Jm53;

        f.getStrain()[0]-=C1Vol*Jm23;
        f.getStrain()[1]-=C2Vol*Jm43;
        f.getStrain()[2]+=K02*x.getStrain()[0] + K12*x.getStrain()[1] - bulkVol*(x.getStrain()[2]-1.);
    }

    void addDForce( Deriv&   df, const Deriv&   dx, const SReal& kfactor, const SReal& /*bfactor*/ ) const
    {
        df.getStrain()[0]+=K02*dx.getStrain()[2]*kfactor;
        df.getStrain()[1]+=K12*dx.getStrain()[2]*kfactor;
        df.getStrain()[2]+=(K02*dx.getStrain()[0]+K12*dx.getStrain()[1]+K22*dx.getStrain()[2])*kfactor;
    }

    MatBlock getK() const
    {
        MatBlock K = MatBlock();
        K[0][2]=K[2][0]=K02;
        K[1][2]=K[2][1]=K12;
        K[2][2]=K22;
        return K;
    }

    MatBlock getC() const
    {
        MatBlock C = MatBlock();
        // singular K!!
        return C;
    }

    MatBlock getB() const
    {
        return MatBlock();
    }
};







} // namespace defaulttype
} // namespace sofa



#endif


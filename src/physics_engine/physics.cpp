#include "physics.h"
#include <cmath>
#include <algorithm>

// Jacobian converts U between Polar and Cartesian
Matrix4d sph_to_cart_Jacobian(double r,double theta,double phi){
    Eigen::Matrix4d J = Eigen::Matrix4d::Zero();
    J(0, 0) = 1.0; // dt/dt is 1

    J(1, 1) = std::sin(theta) * std::cos(phi);
    J(1, 2) = r * std::cos(theta) * std::cos(phi);
    J(1, 3) = -r * std::sin(theta) * std::sin(phi);

    J(2, 1) = std::sin(theta) * std::sin(phi);
    J(2, 2) = r * std::cos(theta) * std::sin(phi);
    J(2, 3) = r * std::sin(theta) * std::cos(phi);

    J(3, 1) = std::cos(theta);
    J(3, 2) = -r * std::sin(theta);
    J(3, 3) = 0.0;
    return J;
}
// Exact same Jacobian but Eigen lets us calculate the inverse directly
Matrix4d cart_to_sph_Jacobian(double r,double theta,double phi){
    return sph_to_cart_Jacobian(r,theta,phi).inverse();
}

State cart_to_sphere(const State& cartState){
    double t = cartState.X[0];
    double x = cartState.X[1];
    double y = cartState.X[2];
    double z = cartState.X[3];

    double r = std::sqrt(x*x+y*y+z*z);
    double phi = std::atan2(y, x);
    double theta = std::acos(std::clamp(z/(r+1e-8), -1.0, 1.0));
    // +1e-8 prevents divide by zero incase particle accidently ends up at origin anc clamp makes sure input to cos-1 is in range

    return State(Vector4d(t, r, theta, phi), cart_to_sph_Jacobian(r,theta,phi)*cartState.U);
}
State sph_to_cart(State& sphState){
    double t = sphState.X[0];
    double r = sphState.X[1];
    double theta = sphState.X[2];
    double phi = sphState.X[3];

    double x = r * std::cos(phi) * std::sin(theta);
    double y = r * std::sin(phi) * std::sin(theta);
    double z = r* std::cos(theta);

    return State(Vector4d(t, x,y,z), sph_to_cart_Jacobian(r,theta,phi)*sphState.U);
}


Vector4d find_acceleration(const State& state){
    // Implement needed
    double r = state.X[1];
    double theta = state.X[2];

    double vt = state.U[0];
    double vr  = state.U[1];
    double vtheta = state.U[2];
    double vphi = state.U[3];

    double sinth = std::sin(theta);
    double costh = std::cos(theta);
    // TODO
    // Replace all  sin cos with precomputes
    // replace r-rs with metric component + 1e-8 for diviide by zero shit

    // T(r) component(radial component)
    double Tr_tt = rs * (r-rs)/(2.0*r*r*r); 
    double Tr_rr = -rs/(2.0*r*(r-rs));
    double Tr_thth = -(r-rs);
    double Tr_phph = -(r-rs)*sinth*sinth;
    //polar component
    double Tth_rth = 1/r;
    double Tth_phph = -sinth*costh;
    //azimuthal component 
    double Tph_rph = 1/r;
    double Tph_thph = costh/(sinth + 1e-8);
    //Time-component
    double Tt_tr = rs/(2*r*(r-rs)); 
    // T(t) component(accelerationequations)
    double at = -2*(Tt_tr)*vt*vr;
    double ar = -(Tr_tt*vt*vt + Tr_rr*vr*vr + Tr_thth*vtheta*vtheta + Tr_phph*vphi*vphi);
    double atheta = -(2*Tth_rth*vr*vtheta + Tth_phph*vphi*vphi);
    double aphi = -(2*Tph_rph*vr*vphi + 2* Tph_thph*vtheta*vphi);


if (r <= rs * 1.001) {
        return Vector4d(0.0, 0.0, 0.0, 0.0); 
    }
    return Vector4d(at, ar, atheta, aphi);
    
}

State create_state_derivate(const State& s) {
    State init;
    init.X = s.U;
    init.U = find_acceleration(s);
    return init;
}
 

State Integrator(const State& s){

//s = s_0 +kt 

    State k1 = create_state_derivate(s);    
    State k2 = create_state_derivate(s + k1 * (dt/2));
    State k3 = create_state_derivate(s + k2 * (dt/2));
    State k4 = create_state_derivate(s + k3 * dt);

    State s_final = s + (dt/6.0 )*(k1 + 2.0*k2 + 2.0*k3 + k4);
    


    return s_final;
}

/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2005, 2007 Klaus Spanderen

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/reference/license.html>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

#include <ql/math/pseudosqrt.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/processes/hestonprocess.hpp>
#include <ql/processes/eulerdiscretization.hpp>

namespace QuantLib {

    HestonProcess::HestonProcess(
                              const Handle<YieldTermStructure>& riskFreeRate,
                              const Handle<YieldTermStructure>& dividendYield,
                              const Handle<Quote>& s0,
                              double v0, double kappa, 
                              double theta, double sigma, double rho)
    : StochasticProcess(boost::shared_ptr<discretization>(
                                                    new EulerDiscretization)),
      riskFreeRate_(riskFreeRate), dividendYield_(dividendYield), s0_(s0),
      v0_   (boost::shared_ptr<Quote>(new SimpleQuote(v0))), 
      kappa_(boost::shared_ptr<Quote>(new SimpleQuote(kappa))), 
      theta_(boost::shared_ptr<Quote>(new SimpleQuote(theta))),
      sigma_(boost::shared_ptr<Quote>(new SimpleQuote(sigma))), 
      rho_  (boost::shared_ptr<Quote>(new SimpleQuote(rho))) {

        registerWith(riskFreeRate_);
        registerWith(dividendYield_);
        registerWith(s0_);
        registerWith(v0_);
        registerWith(kappa_);
        registerWith(theta_);
        registerWith(sigma_);
        registerWith(rho_);
    }

    Size HestonProcess::size() const {
        return 2;
    }

    Disposable<Array> HestonProcess::initialValues() const {
        Array tmp(2);
        tmp[0] = s0_->value();
        tmp[1] = v0_->value();
        return tmp;
    }

    Disposable<Array> HestonProcess::drift(Time t, const Array& x) const {
        Array tmp(2);
        const Real vol = (x[1] > 0.0) ? std::sqrt(x[1]) : 0.0;
        tmp[0] = riskFreeRate_->forwardRate(t, t, Continuous)
               - dividendYield_->forwardRate(t, t, Continuous)
               - 0.5 * vol * vol;

        // A plain vanilla discretization schema that
        // seems to produce the smallest bias.
        // See Lord, R., R. Koekkoek and D. van Dijk (2006),
        // "A Comparison of biased simulation schemes for
        //  stochastic volatility models", Working Paper, Tinbergen Institute
        tmp[1] = kappa_->value()*(theta_->value() - vol*vol);
        return tmp;
    }

    Disposable<Matrix> HestonProcess::diffusion(Time, const Array& x) const {
        /* the correlation matrix is
           |  1   rho |
           | rho   1  |
           whose square root (which is used here) is
           |  1          0       |
           | rho   sqrt(1-rho^2) |
        */
        Matrix tmp(2,2);
        const Real rho = rho_->value();
        const Real sigma1 = (x[1] > 0.0) ? std::sqrt(x[1]) : 0.0;
        const Real sigma2 = sigma_->value() * sigma1;
        tmp[0][0] = sigma1;      tmp[0][1] = 0.0;
        tmp[1][0] = rho*sigma2;  tmp[1][1] = std::sqrt(1.0-rho*rho)*sigma2;
        return tmp;
    }

    Disposable<Array> HestonProcess::apply(const Array& x0,
                                           const Array& dx) const {
        Array tmp(2);
        tmp[0] = x0[0] * std::exp(dx[0]);
        tmp[1] = x0[1] + dx[1];
        return tmp;
    }

    const RelinkableHandle<Quote>& HestonProcess::v0() const {
        return v0_;
    }

    const RelinkableHandle<Quote>& HestonProcess::rho() const {
        return rho_;
    }

    const RelinkableHandle<Quote>& HestonProcess::kappa() const {
        return kappa_;
    }

    const RelinkableHandle<Quote>& HestonProcess::theta() const {
        return theta_;
    }

    const RelinkableHandle<Quote>& HestonProcess::sigma() const {
        return sigma_;
    }

    const Handle<Quote>& HestonProcess::s0() const {
        return s0_;
    }

    const Handle<YieldTermStructure>& HestonProcess::dividendYield() const {
        return dividendYield_;
    }

    const Handle<YieldTermStructure>& HestonProcess::riskFreeRate() const {
        return riskFreeRate_;
    }

    Time HestonProcess::time(const Date& d) const {
        return riskFreeRate_->dayCounter().yearFraction(
                                           riskFreeRate_->referenceDate(), d);
    }

}
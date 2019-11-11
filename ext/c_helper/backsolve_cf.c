#include <stdio.h>
#include <math.h>  // for pow() function in IRR calculation
#include <ruby.h>

#define ABS(x) (((x)<0.0) ? (-(x)) : (x))


/* _compute_pv()
 * internal function that computes the sum of the discounted present values of a stream of cash flows, using
 * the same logic as the Ruby code in the app
 *
 * assumes that the arrays are properly allocated and are num_cfs in length
 * assumes that the discount rate and date calculations won't result in a denominator of 0
 */
double _compute_pv(double *cfs, double *dates, double *libor, long num_cfs, char is_clean, double accrued_interest, double year_convention, double spread) {
  double discount_rate, discount_factor = 1.0, prev_cumul_date = 0.0, cumul_pv = 0.0;
  long t;
  
  if (num_cfs > 0) {
    // loop through cash flows and discount
    for (t = 0; t < num_cfs; t++) {
      discount_rate = libor[t] + spread;
      discount_factor /= (1.0 + discount_rate * (dates[t] - prev_cumul_date) / year_convention);
      cumul_pv += cfs[t] * discount_factor;
      prev_cumul_date = dates[t];
    }
  
    if (is_clean)
      cumul_pv -= accrued_interest;
  
    return cumul_pv;
  } else { // no dates or cash flows to discount
    return -997.0;
  }
}


/* _compute_pv_for_irr()
 * internal function that computes the sum of the discounted present values of a stream of cash flows, using
 * the same logic as the Ruby code in the app
 * 
 * Logic is Actual/365, annual compounded discount rates
 *
 * assumes that the arrays are properly allocated and are num_cfs in length
 * assumes that the discount rate and date calculations won't result in a denominator of 0
 */
double _compute_pv_for_irr(double *cfs, double *dates, long num_cfs, char is_clean, double accrued_interest, double irr) {
  double discount_factor = 1.0, orig_date = 0.0, cumul_pv = 0.0;
  double year_convention = 365.0;
  long t;
  
  if (num_cfs > 0) {
    orig_date = dates[0];

    // loop through cash flows and discount
    for (t = 0; t < num_cfs; t++) {
      discount_factor = 1.0 / pow(1.0 + irr, (dates[t] - orig_date) / year_convention);
      cumul_pv += cfs[t] * discount_factor;
    }
  
    if (is_clean)
      cumul_pv -= accrued_interest;

    return cumul_pv;
  } else { // no dates to discount
    return -997.0;
  }  
}


/* _backsolve_cf()
 * internal function that applies a Newton-Raphson algorithm to find, for a given set of cash flows and a target NPV,
 * what discount *spread* (i.e. spread over libor) will get to that NPV.
 *
 * note that the NPV isn't a price (% of par), but rather a total dollar amount
 *
 * if you have a fixed-rate loan, just pass a string of 0's in the libor array and it will return a yield instead of a spread
 *
 * assumes that arrays are allocated and num_cfs in length
 * assumes that -999.0 and -998.0 will never be valid return values for spreads
 */
double _backsolve_cf(double *cfs, double *dates, double *libor, long num_cfs, double target_px, double res, long max_tries, char is_clean, double accrued_interest, double year_convention) {
  double x_n_minus_1 = 0.06; // starting point of 6%
  double x_n = x_n_minus_1 + 0.0025;
  double x_n_plus_1;
  double f_n_minus_1, f_n;
  
  f_n_minus_1 = target_px - _compute_pv(cfs, dates, libor, num_cfs, is_clean, accrued_interest, year_convention, x_n_minus_1);
  f_n         = target_px - _compute_pv(cfs, dates, libor, num_cfs, is_clean, accrued_interest, year_convention, x_n        );
  
  long trials = 0;
  
  while (ABS(f_n) > res && trials < max_tries) {
    if (f_n == f_n_minus_1) {
      return -999.0;
      // ERROR! Can't divide by 0
    }
    x_n_plus_1    = x_n - f_n * (x_n - x_n_minus_1) / (f_n - f_n_minus_1);
    x_n_minus_1   = x_n;
    x_n           = x_n_plus_1;

    f_n_minus_1   = f_n;   // previous result
    f_n           = target_px - _compute_pv(cfs, dates, libor, num_cfs, is_clean, accrued_interest, year_convention, x_n);
    
    trials++;
  }
  
  if (trials >= max_tries)
    return -998.0;
  
  return x_n;
}


/* _backsolve_irr()
 * internal function that applies a Newton-Raphson algorithm to find, for a given set of cash flows,
 * what IRR will get to an NPV of 0.0.
 *
 * note that the NPV isn't a price (% of par), but rather a total dollar amount
 *
 * assumes that arrays are allocated and num_cfs in length
 * assumes that -999.0 and -998.0 will never be valid return values for spreads
 */
double _backsolve_irr(double *cfs, double *dates, long num_cfs, double res, long max_tries, char is_clean, double accrued_interest) {
  double x_n_minus_1 = 0.06; // starting point of 6%
  double x_n = x_n_minus_1 + 0.0025;
  double x_n_plus_1;
  double f_n_minus_1, f_n;
  double target_px = 0.0;
  
  f_n_minus_1 = target_px - _compute_pv_for_irr(cfs, dates, num_cfs, is_clean, accrued_interest, x_n_minus_1);
  f_n         = target_px - _compute_pv_for_irr(cfs, dates, num_cfs, is_clean, accrued_interest, x_n        );
  
  long trials = 0;
  
  while (ABS(f_n) > res && trials < max_tries) {
    if (f_n == f_n_minus_1) {
      return -999.0;
      // ERROR! Can't divide by 0
    }
    x_n_plus_1    = x_n - f_n * (x_n - x_n_minus_1) / (f_n - f_n_minus_1);
    x_n_minus_1   = x_n;
    x_n           = x_n_plus_1;

    f_n_minus_1   = f_n;   // previous result
    f_n           = target_px - _compute_pv_for_irr(cfs, dates, num_cfs, is_clean, accrued_interest, x_n);
    
    trials++;
  }
  
  if (trials >= max_tries)
    return -998.0;
  
  return x_n;
}


/* backsolve_cf
 * exported function that is called from Ruby to compute the backsolved spread or yield; main function is data wrangling
 * from Ruby to C and vice versa, then calling the internal functions
 *
 * assumes that is_clean is a boolean True or False
 * assumes that the Ruby arrays cfs, dates, and libor are of length num_cfs
 * assumes that the numeric classes are as per below, and Ruby won't, e.g., throw in a BigNum or some other class
 * assumes that the Check_Type code will not just raise a Ruby exception, but will also safely exit the function without
 *  allocating bad memory etc. and proceeding with the C code further down
 */
VALUE backsolve_cf(VALUE _self, VALUE cfs, VALUE dates, VALUE libor, VALUE num_cfs, VALUE target_px, VALUE res, VALUE max_tries, VALUE is_clean, VALUE accrued_interest, VALUE year_convention) {
  Check_Type(cfs,               T_ARRAY);
  Check_Type(dates,             T_ARRAY);
  Check_Type(libor,             T_ARRAY);
  Check_Type(num_cfs,           T_FIXNUM);
  Check_Type(target_px,         T_FLOAT);
  Check_Type(res,               T_FLOAT);
  Check_Type(max_tries,         T_FIXNUM);
  //Check_Type(is_clean,   T_TRUE); Ruby doesn't have a Boolean class
  Check_Type(accrued_interest,  T_FLOAT);
  Check_Type(year_convention,   T_FLOAT);
  
  double *c_cfs, *c_dates, *c_libor;
  double prev_date;
  long c_num_cfs = NUM2LONG(num_cfs);
  
  if (c_num_cfs < 1) {
    rb_raise(rb_eRuntimeError, "valid array of cash flows must have at least one entry");
    return Qnil;
  }
  
  // allocate memory
  c_cfs   = malloc(c_num_cfs * sizeof(double)); if (c_cfs == NULL) { rb_raise(rb_eNoMemError, "failed to allocate memory for c_cfs"); return Qnil; }
  c_dates = malloc(c_num_cfs * sizeof(double)); if (c_cfs == NULL) { free(c_cfs); rb_raise(rb_eNoMemError, "failed to allocate memory for c_dates"); return Qnil; }
  c_libor = malloc(c_num_cfs * sizeof(double)); if (c_cfs == NULL) { free(c_cfs); free(c_dates); rb_raise(rb_eNoMemError, "failed to allocate memory for c_libor"); return Qnil; }
  
  // TODO: handle errors, what if the Ruby array is not the right length?
  long i;
  prev_date = 0.0;
  for (i = 0; i < c_num_cfs; i++) {
    c_cfs[i]   = NUM2DBL(rb_ary_entry(cfs,   i));
    c_dates[i] = NUM2DBL(rb_ary_entry(dates, i));
    if (c_dates[i] <= prev_date) {
      free(c_cfs); free(c_dates); free(c_libor);
      rb_raise(rb_eRuntimeError, "dates must contain a list of monotonically increasing values, starting at a value > 0");
      return Qnil;
    }
    prev_date = c_dates[i];
    c_libor[i] = NUM2DBL(rb_ary_entry(libor, i));
  }
  
  // call internal function to compute result
  double c_result = _backsolve_cf(c_cfs, c_dates, c_libor,
    c_num_cfs,
    NUM2DBL(target_px),
    NUM2DBL(res),
    NUM2LONG(max_tries),
    TYPE(is_clean) == T_TRUE ? 1 : 0,
    NUM2DBL(accrued_interest),
    NUM2DBL(year_convention));
  
    // free memory
  free(c_cfs);
  free(c_dates);
  free(c_libor);
  
  if (c_result == -999.0) {
    rb_raise(rb_eZeroDivError, "value doesn't change when yield is sensitized");
    return Qnil;
  } else if (c_result == -998.0) {
    rb_raise(rb_eRuntimeError, "failed to converge");
    return Qnil;
  }
  
  VALUE result = rb_float_new(c_result);
  return result;
}


/* backsolve_cf
 * exported function that is called from Ruby to compute the backsolved spread or yield; main function is data wrangling
 * from Ruby to C and vice versa, then calling the internal functions
 *
 * assumes that is_clean is a boolean True or False
 * assumes that the Ruby arrays cfs, dates, and libor are of length num_cfs
 * assumes that the numeric classes are as per below, and Ruby won't, e.g., throw in a BigNum or some other class
 * assumes that the Check_Type code will not just raise a Ruby exception, but will also safely exit the function without
 *  allocating bad memory etc. and proceeding with the C code further down
 */
VALUE backsolve_irr(VALUE _self, VALUE cfs, VALUE dates, VALUE num_cfs, VALUE res, VALUE max_tries, VALUE is_clean, VALUE accrued_interest) {
  Check_Type(cfs,               T_ARRAY);
  Check_Type(dates,             T_ARRAY);
  Check_Type(num_cfs,           T_FIXNUM);
  Check_Type(res,               T_FLOAT);
  Check_Type(max_tries,         T_FIXNUM);
  //Check_Type(is_clean,   T_TRUE); Ruby doesn't have a Boolean class
  Check_Type(accrued_interest,  T_FLOAT);
  
  double *c_cfs, *c_dates;
  double prev_date;
  long c_num_cfs = NUM2LONG(num_cfs);
  
  if (c_num_cfs < 1) {
    rb_raise(rb_eRuntimeError, "valid array of cash flows must have at least one entry");
    return Qnil;
  }
  
  // allocate memory
  c_cfs   = malloc(c_num_cfs * sizeof(double)); if (c_cfs == NULL) { rb_raise(rb_eNoMemError, "failed to allocate memory for c_cfs"); return Qnil; }
  c_dates = malloc(c_num_cfs * sizeof(double)); if (c_cfs == NULL) { free(c_cfs); rb_raise(rb_eNoMemError, "failed to allocate memory for c_dates"); return Qnil; }
  
  // TODO: handle errors, what if the Ruby array is not the right length?
  long i;
  prev_date = 0.0;
  for (i = 0; i < c_num_cfs; i++) {
    c_cfs[i]   = NUM2DBL(rb_ary_entry(cfs,   i));
    c_dates[i] = NUM2DBL(rb_ary_entry(dates, i));
    if (c_dates[i] <= prev_date) {
      free(c_cfs); free(c_dates);
      rb_raise(rb_eRuntimeError, "dates must contain a list of monotonically increasing values, starting at a value > 0");
      return Qnil;
    }
    prev_date = c_dates[i];
  }
  
  // call internal function to compute result
  double c_result = _backsolve_irr(c_cfs, c_dates,
    c_num_cfs,
    NUM2DBL(res),
    NUM2LONG(max_tries),
    TYPE(is_clean) == T_TRUE ? 1 : 0,
    NUM2DBL(accrued_interest));
  
    // free memory
  free(c_cfs);
  free(c_dates);
  
  if (c_result == -999.0) {
    rb_raise(rb_eZeroDivError, "value doesn't change when yield is sensitized");
    return Qnil;
  } else if (c_result == -998.0) {
    rb_raise(rb_eRuntimeError, "failed to converge");
    return Qnil;
  }
  
  VALUE result = rb_float_new(c_result);
  return result;
}


void Init_c_helper() {
  VALUE mod = rb_define_module("CHelper");
  rb_define_module_function(mod, "backsolve_cf", backsolve_cf, 10);
  rb_define_module_function(mod, "backsolve_irr", backsolve_irr, 7);
}

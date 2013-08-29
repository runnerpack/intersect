/*
	Based on Nathan "noonat" Ostgard's "Intersect" CoffeeScript library

	Converted into a Ruby C-extension by Amos Bieler (August, 2013)

	NB: "slood" stands for "Saved Length is Out-Of-Date".
*/

#include "ruby.h"
#include <math.h>

static VALUE
	m_Intersect,
	c_Point,
	c_Hit,
	c_Sweep,
	c_AABB;

/*/ Class definitions /////////////////////*/

/*/ Point class ///////////////////////////*/

/* Thanks, Pythagoras, ya kooky old Samian! */
#define LENGTH(x,y) rb_float_new( sqrt( NUM2DBL(x) * NUM2DBL(x) + NUM2DBL(y) * NUM2DBL(y) ) )

/* Set the Point's X value */
static VALUE rb_point_x_set(VALUE self, VALUE val)
{
	rb_iv_set(self, "@x", val);
	rb_iv_set(self, "@slood", Qtrue);
	return self;
}

/* Set the Point's Y value */
static VALUE rb_point_y_set(VALUE self, VALUE val)
{
	rb_iv_set(self, "@y", val);
	rb_iv_set(self, "@slood", Qtrue);
	return self;
}

/* Recalculate (if necessary) and return the Point's length */
static VALUE rb_point_length_get(VALUE self)
{
	VALUE length;

	if (rb_iv_get(self, "@slood") == Qtrue) {
		length = LENGTH(rb_iv_get(self, "@x"), rb_iv_get(self, "@y"));
		rb_iv_set(self, "@slood", Qfalse);
	} else {
		length = rb_iv_get(self, "@length");
	}
	rb_iv_set(self, "@length", length);

	return length;
}

/* Return a copy of the Point normalized to unit length */
static VALUE rb_point_normalize(VALUE self)
{
	VALUE
		length = rb_point_length_get(self),
		point,
		inv_length;

	if (length <= 0) return Qnil;

	point = rb_class_new_instance(0, NULL, c_Point);

	// I'm not sure all this inversion stuff is necessary...
	inv_length = 1.0 / NUM2DBL(length);
	rb_iv_set(point, "@x", rb_float_new(NUM2DBL(rb_iv_get(self, "@x")) * inv_length));
	rb_iv_set(point, "@y", rb_float_new(NUM2DBL(rb_iv_get(self, "@y")) * inv_length));
	rb_iv_set(point, "@slood", Qtrue);

	return point;
}

/* Normalize the Point in place */
static VALUE rb_point_normalize_bang(VALUE self)
{
	VALUE
		length = rb_point_length_get(self),
		inv_length;

	if (length <= 0) return Qnil;

	// I'm not sure all this inversion stuff is necessary...
	inv_length = 1.0 / NUM2DBL(length);
	rb_iv_set(self, "@x", rb_float_new(NUM2DBL(rb_iv_get(self, "@x")) * inv_length));
	rb_iv_set(self, "@y", rb_float_new(NUM2DBL(rb_iv_get(self, "@y")) * inv_length));
	rb_iv_set(self, "@slood", Qtrue);

	return self;
}

/* Initialize a new Point instance */
static VALUE rb_point_init(int argc, VALUE* argv, VALUE self)
{

    VALUE
		x = INT2NUM(0),
		y = INT2NUM(0);

    if (argc > 2) {  // there should only be 0, 1 or 2 arguments
        rb_raise(rb_eArgError, "Wrong number of arguments");
    } else if (argc > 0) {
		// x = INT2NUM(argv[0]);
		// if (argc > 1) y = INT2NUM(argv[1]);
		x = argv[0];
		if (argc > 1) y = argv[1];
    }

	rb_iv_set(self, "@x", x);
	rb_iv_set(self, "@y", y);
	rb_iv_set(self, "@length", LENGTH(x, y));
	rb_iv_set(self, "@slood", Qfalse);

	return self;
}


/*/ Hit class /////////////////////////////*/

static VALUE rb_hit_init(VALUE self)
{
	rb_iv_set(self, "@pos", rb_class_new_instance(0, NULL, c_Point));
	rb_iv_set(self, "@delta", rb_class_new_instance(0, NULL, c_Point));
	rb_iv_set(self, "@normal", rb_class_new_instance(0, NULL, c_Point));
	rb_iv_set(self, "@time", Qnil);

	return self;
}

/*/ Sweep class ///////////////////////////*/

static VALUE rb_sweep_init(VALUE self)
{
	rb_iv_set(self, "@hit", Qnil);
	rb_iv_set(self, "@pos", rb_class_new_instance(0, NULL, c_Point));

	return self;
}


/*/ AABB class ////////////////////////////*/

/* Set the AABB's "radius" */
static VALUE rb_aabb_half_set(VALUE self, VALUE val)
{
    VALUE
		half = Qnil,
		radius[2];

	radius[0] = INT2NUM(abs(NUM2INT(rb_iv_get(val, "@x"))));
	radius[1] = INT2NUM(abs(NUM2INT(rb_iv_get(val, "@y"))));

	half = rb_class_new_instance(2, radius, c_Point);
	rb_iv_set(self, "@half", half);

	return self;
}

static VALUE rb_aabb_intersect_point(VALUE self, VALUE point)
{
	VALUE
		hit,
		dx, dy,
		px, py,
		sx, sy;

	// Find the overlap for the X axis
	dx = rb_iv_get(point, "@x") - rb_iv_get(rb_iv_get(self, "@pos"), "@x");
    px = rb_iv_get(rb_iv_get(self, "@half"), "@x") - rb_funcall(dx, rb_intern("abs"), 0);
	if (NUM2DBL(px) <= 0) return Qnil;

	// Find the overlap for the Y axis
	dy = rb_iv_get(point, "@y") - rb_iv_get(rb_iv_get(self, "@pos"), "@y");
    py = rb_iv_get(rb_iv_get(self, "@half"), "@y") - rb_funcall(dy, rb_intern("abs"), 0);
	if (NUM2DBL(py) <= 0) return Qnil;

	// Use the axis with the smallest overlap
	hit = rb_class_new_instance(0, NULL, c_Hit);
	if (px < py) {
		sx = rb_funcall(dx, rb_intern("<=>"), 1, INT2NUM(0));
		rb_iv_set(rb_iv_get(hit, "@delta"), "@x", rb_float_new(NUM2DBL(px) * NUM2DBL(sx)));
		rb_iv_set(rb_iv_get(hit, "@normal"), "@x", sx);
		rb_iv_set(rb_iv_get(hit, "@pos"), "@x", NUM2DBL(rb_iv_get(rb_iv_get(self, "@pos"), "@x")) + (NUM2DBL(rb_iv_get(rb_iv_get(self, "@half"), "@x")) * NUM2DBL(sx)));
		rb_iv_set(rb_iv_get(hit, "@pos"), "@y", rb_iv_get(point, "@y"));
	} else {
		sy = rb_funcall(dy, rb_intern("<=>"), 1, INT2NUM(0));
		rb_iv_set(rb_iv_get(hit, "@delta"), "@y", rb_float_new(NUM2DBL(py) * NUM2DBL(sy)));
		rb_iv_set(rb_iv_get(hit, "@normal"), "@y", sy);
		rb_iv_set(rb_iv_get(hit, "@pos"), "@x", rb_iv_get(point, "@x"));
		rb_iv_set(rb_iv_get(hit, "@pos"), "@y", NUM2DBL(rb_iv_get(rb_iv_get(self, "@pos"), "@y")) + (NUM2DBL(rb_iv_get(rb_iv_get(self, "@half"), "@y")) * NUM2DBL(sy)));
	}

	return hit;
}

static VALUE rb_aabb_intersect_segment(VALUE self, VALUE segment)
{
	return Qnil;
}

static VALUE rb_aabb_intersect_aabb(VALUE self, VALUE other)
{
	return Qnil;
}

static VALUE rb_aabb_sweep_aabb(VALUE self, VALUE other)
{
	return Qnil;
}

static VALUE rb_aabb_init(int argc, VALUE* argv, VALUE self)
{

    VALUE
		pos = Qnil,
		half = Qnil,
		radius[2] = { INT2NUM(1), INT2NUM(1) };

    if (argc > 2) {  // there should only be 0, 1 or 2 arguments
        rb_raise(rb_eArgError, "Wrong number of arguments");
    } else if (argc > 0) {
		pos = argv[0];
		if (argc > 1) half = argv[1];
    } else {
		pos = rb_class_new_instance(0, NULL, c_Point);
		half = rb_class_new_instance(2, radius, c_Point);
	}

	rb_iv_set(self, "@pos", pos);
	rb_aabb_half_set(self, half);

	return self;
}


/*/ Extension Entry Point /////////////////*/

void Init_intersect()
{
	/* Define our module */
	m_Intersect = rb_define_module("Intersect");

	/* Define our classes */

	/* A 2D point */
	c_Point = rb_define_class_under(m_Intersect, "Point", rb_cObject);
	rb_define_method(c_Point, "initialize", rb_point_init, -1);
	rb_define_method(c_Point, "x=", rb_point_x_set, 1);
	rb_define_method(c_Point, "y=", rb_point_y_set, 1);
	rb_define_method(c_Point, "length", rb_point_length_get, 0);
	rb_define_method(c_Point, "normalize", rb_point_normalize, 0);
	rb_define_method(c_Point, "normalize!", rb_point_normalize_bang, 0);

	rb_define_attr(c_Point, "x", TRUE, FALSE);
	rb_define_attr(c_Point, "y", TRUE, FALSE);

	/* The result of intersection tests */
	c_Hit = rb_define_class_under(m_Intersect, "Hit", rb_cObject);
	rb_define_method(c_Hit, "initialize", rb_hit_init, 0);

	rb_define_attr(c_Hit, "pos", TRUE, TRUE);		/* The point where the collision occurred */
	rb_define_attr(c_Hit, "delta", TRUE, TRUE);		/* The overlap for the collision */
	rb_define_attr(c_Hit, "normal", TRUE, TRUE);	/* The surface normal of the hit edge */
	rb_define_attr(c_Hit, "time", TRUE, TRUE);		/* Optional; used for segment intersection */

	/* The result of sweep tests */
	c_Sweep = rb_define_class_under(m_Intersect, "Sweep", rb_cObject);
	rb_define_method(c_Sweep, "initialize", rb_sweep_init, 0);

	rb_define_attr(c_Sweep, "hit", TRUE, TRUE);		/* The collision result (if one occurred) */
	rb_define_attr(c_Sweep, "pos", TRUE, TRUE);		/* The farthest point prior to collision */

	/* An axis-aligned bounding box */
	c_AABB = rb_define_class_under(m_Intersect, "AABB", rb_cObject);
	rb_define_method(c_AABB, "initialize", rb_aabb_init, -1);
	rb_define_method(c_AABB, "half=", rb_aabb_half_set, 1);

	/* The actual intersection tests */
	rb_define_method(c_AABB, "intersect_point", rb_aabb_intersect_point, 1);
	rb_define_method(c_AABB, "intersect_segment", rb_aabb_intersect_segment, 1);
	rb_define_method(c_AABB, "intersect_AABB", rb_aabb_intersect_aabb, 1);
	rb_define_method(c_AABB, "sweep_AABB", rb_aabb_sweep_aabb, 1);

	rb_define_attr(c_AABB, "pos", TRUE, TRUE);
	rb_define_attr(c_AABB, "half", TRUE, FALSE);
}

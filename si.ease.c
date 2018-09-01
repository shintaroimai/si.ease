/* ============================================================
 * si.ease v1.0
 *
 * Open source under the MIT License and the 3-Clause BSD License.
 *
 * Copyright © 2018 Shintaro Imai
 * All rights reserved.
 * https://raw.github.com/shintaroimai/siease/master/LICENSE
 * ======================================================== */

#include "ext.h"
#include "ext_obex.h"

#define PI 4*atan(1.0)
#define DEFAULT_GRAIN 20
#define MIN_GRAIN 1

////////////////////////// object struct
typedef struct _easing
{
    t_object e_ob;
    t_atom e_args[3];
    double e_t;//time
    double e_b;//beginning position
    double e_c;//total change
    double e_d;//duration
    double e_dd;//temporal duration
    double e_dest;//destination
    double e_current;//current position
    long e_mode[2];//easing mode (array)
    long e_modenum;//easing mode (int)
    long e_flag;
    void *e_clock;//pointer to clock object
    double e_interval;//grain size in milliseconds
    double e_count;//counter
    void *e_out;
    void *e_bangout;
} t_easing;

///////////////////////// function prototypes
/*-----------------------------------------------------------*/
void *easing_new(t_symbol *s, long argc, t_atom *argv);
void easing_free(t_easing *x);
void easing_int_b(t_easing *x, long value);
void easing_float_b(t_easing *x, double value);
void easing_in1_d(t_easing *x, long value);
void easing_ft1_d(t_easing *x, double value);
void easing_in2_interval(t_easing *x, long value);
void easing_ft2_interval(t_easing *x, double value);
void easing_mode(t_easing *x, t_symbol *s, long argc, t_atom *argv);
void easing_list(t_easing *x, t_symbol *s, long argc, t_atom *argv);
void easing_update(t_easing *x);
void easing_round(t_easing *x);
void easing_culc(t_easing *x);
void easing_bang(t_easing *x);
/*-----------------------------------------------------------*/
void easing_quadIn(t_easing *x);
void easing_quadOut(t_easing *x);
void easing_quadInOut(t_easing *x);
void easing_cubicIn(t_easing *x);
void easing_cubicOut(t_easing *x);
void easing_cubicInOut(t_easing *x);
void easing_quartIn(t_easing *x);
void easing_quartOut(t_easing *x);
void easing_quartInOut(t_easing *x);
void easing_quintIn(t_easing *x);
void easing_quintOut(t_easing *x);
void easing_quintInOut(t_easing *x);
void easing_sinIn(t_easing *x);
void easing_sinOut(t_easing *x);
void easing_sinInOut(t_easing *x);
void easing_expIn(t_easing *x);
void easing_expOut(t_easing *x);
void easing_expInOut(t_easing *x);
void easing_circIn(t_easing *x);
void easing_circOut(t_easing *x);
void easing_circInOut(t_easing *x);
/*-----------------------------------------------------------*/
void clock_function(t_easing *x);
void metro_start(t_easing *x);
void metro_stop(t_easing *x);
void easing_stop(t_easing *x);
void easing_set(t_easing *x, t_symbol *s, long argc, t_atom *argv);
void easing_print(t_easing *x);
/*-----------------------------------------------------------*/
void easing_assist(t_easing *x, void *b, long m, long a, char *s);
/*-----------------------------------------------------------*/

//////////////////////// global class pointer variable
void *easing_class;

void ext_main(void *r)
{
    t_class *c = class_new("si.ease", (method)easing_new, (method)easing_free, sizeof(t_easing), (method)0L, A_GIMME, 0);
    
    class_addmethod(c, (method)easing_int_b, "int", A_LONG, 0);
    class_addmethod(c, (method)easing_float_b, "float", A_FLOAT, 0);
    class_addmethod(c, (method)easing_in1_d, "in1", A_LONG, 0);
    class_addmethod(c, (method)easing_ft1_d, "ft1", A_FLOAT, 0);
    class_addmethod(c, (method)easing_in2_interval, "in2", A_LONG, 0);
    class_addmethod(c, (method)easing_ft2_interval, "ft2", A_FLOAT, 0);
    class_addmethod(c, (method)easing_mode, "mode", A_GIMME, 0);
    class_addmethod(c, (method)easing_list, "list", A_GIMME, 0);
    class_addmethod(c, (method)easing_bang, "bang", 0);
    class_addmethod(c, (method)easing_stop, "stop", A_GIMME, 0);
    class_addmethod(c, (method)easing_set, "set", A_GIMME, 0);
    class_addmethod(c, (method)easing_print, "print", A_GIMME, 0);
    
    class_addmethod(c, (method)easing_assist, "assist", A_CANT, 0);
    
    class_register(CLASS_BOX, c);
    easing_class = c;
    
    post("si.ease 1.0; 2018 Shintaro Imai, based on Robert Penner’s easing formulas");
}

void *easing_new(t_symbol *s, long argc, t_atom *argv)
{
    t_easing *x = (t_easing *)object_alloc(easing_class);
    x->e_interval = DEFAULT_GRAIN;
    long i;
    
    for (i=0; i < argc; i++)
    {
        if(argv[i].a_type==A_LONG)
            x->e_interval = argv[i].a_w.w_long;
        else if(argv[i].a_type==A_FLOAT)
            x->e_interval = argv[i].a_w.w_float;
    }
    
    x->e_clock = clock_new(x, (method)clock_function); // create the metronome clock
    
    floatin(x,2);
    floatin(x,1);
    
    x->e_bangout = bangout(x);
    x->e_out = floatout(x);
    
    x->e_t = x->e_b = x->e_c = x->e_d = x->e_dest = x->e_current = 0;
    
    x->e_mode[0] = x->e_mode[1] = 0;
    x->e_flag = 0;
    easing_mode(x, s, argc, argv);
    
    return (x);
}

void easing_free(t_easing *x)
{
    clock_unset(x->e_clock); // remove the clock routine from the scheduler
    freeobject((t_object *)x->e_clock);
}

/*-----------------------------------------------------------*/

void easing_assist(t_easing *x, void *b, long m, long a, char *s)
{
    if (m == ASSIST_INLET)
    {
        switch (a)
        {
            case 0: sprintf(s, "%s", "destination value of easing");
                break;
            case 1: sprintf(s, "%s", "total easing duration in milliseconds");
                break;
            case 2: sprintf(s, "%s", "time grain in milliseconds");
                break;
        }
    }
    else if (m == ASSIST_OUTLET)
        switch (a)
    {
        case 0: sprintf(s, "%s", "eased output");
            break;
        case 1: sprintf(s, "%s", "signals end of easing");
    }
}

/*-----inlets----------------------------------------------*/

void easing_int_b(t_easing *x, long value)
{
    if(!x->e_flag)
    {
        metro_stop(x);
        x->e_b = value;
        outlet_float(x->e_out, x->e_b);
    }
    else
    {
        x->e_dest = value;
        easing_update(x);
        metro_start(x);
        x->e_flag = 0;
    }
}

void easing_float_b(t_easing *x, double value)
{
    if(!x->e_flag)
    {
        metro_stop(x);
        x->e_b = value;
        outlet_float(x->e_out, x->e_b);
    }
    else
    {
        x->e_dest = value;
        easing_update(x);
        metro_start(x);
        x->e_flag = 0;
    }
}

void easing_in1_d(t_easing *x, long value)
{
    x->e_d = value;
    easing_round(x);
    x->e_flag = 1;
}

void easing_ft1_d(t_easing *x, double value)
{
    x->e_d = value;
    easing_round(x);
    x->e_flag = 1;
}

void easing_in2_interval(t_easing *x, long value)
{
    if (value < 1)
        x->e_interval = 1;
    else
        x->e_interval = value;
}

void easing_ft2_interval(t_easing *x, double value)
{
    if (value < 1)
        x->e_interval = 1;
    else
        x->e_interval = value;
}

void easing_list(t_easing *x, t_symbol *s, long argc, t_atom *argv)
{
    if (argc == 2)
    {
        if(argv[0].a_type==A_LONG)
            x->e_dest = argv[0].a_w.w_long;
        else if(argv[0].a_type==A_FLOAT)
            x->e_dest = argv[0].a_w.w_float;
        if(argv[1].a_type==A_LONG)
            x->e_d = argv[1].a_w.w_long;
        else if(argv[1].a_type==A_FLOAT)
            x->e_d = argv[1].a_w.w_float;
        easing_update(x);
        easing_round(x);
        metro_start(x);
    }
}

void easing_mode(t_easing *x, t_symbol *s, long argc, t_atom *argv)
{
    long i;
    for(i=0; i < argc; i++)
    {
        if(argv[i].a_type==A_SYM)
        {
            if(strcmp(argv[i].a_w.w_sym->s_name, "quad")==0)
                x->e_mode[0] = 0;
            else if(strcmp(argv[i].a_w.w_sym->s_name, "cubic")==0)
                x->e_mode[0] = 1;
            else if(strcmp(argv[i].a_w.w_sym->s_name, "quart")==0)
                x->e_mode[0] = 2;
            else if(strcmp(argv[i].a_w.w_sym->s_name, "quint")==0)
                x->e_mode[0] = 3;
            else if(strcmp(argv[i].a_w.w_sym->s_name, "sin")==0)
                x->e_mode[0] = 4;
            else if(strcmp(argv[i].a_w.w_sym->s_name, "exp")==0)
                x->e_mode[0] = 5;
            else if(strcmp(argv[i].a_w.w_sym->s_name, "circ")==0)
                x->e_mode[0] = 6;
            //
            else if(strcmp(argv[i].a_w.w_sym->s_name, "in")==0)
                x->e_mode[1] = 0;
            else if(strcmp(argv[i].a_w.w_sym->s_name, "out")==0)
                x->e_mode[1] = 1;
            else if(strcmp(argv[i].a_w.w_sym->s_name, "inout")==0)
                x->e_mode[1] = 2;
            else
            {
                post("no such mode! set to quad in");
                x->e_mode[0] = 0;
                x->e_mode[1] = 0;
            }
        }
    }
    x->e_modenum = x->e_mode[0]*10 + x->e_mode[1];
    //    post("%d", x->e_modenum);
}

void easing_bang(t_easing *x)
{
    outlet_float(x->e_out, x->e_b);
}

/*-----functions-------------------------------------------*/

void easing_update(t_easing *x)
{
    x->e_current = x->e_b;
    x->e_c = x->e_dest - x->e_b;
}

void easing_round(t_easing *x)
{
    long i = 0;
    if(x->e_d < x->e_interval)
        x->e_d = x->e_interval;
    i = x->e_d / x->e_interval;
    x->e_d = i * x->e_interval;
}

void easing_set(t_easing *x, t_symbol *s, long argc, t_atom *argv)
{
    metro_stop(x);
    if(argv[0].a_type == A_LONG)
        x->e_b = argv[0].a_w.w_long;
    else if(argv[0].a_type == A_FLOAT)
        x->e_b = argv[0].a_w.w_float;
}

void easing_culc(t_easing *x)
{
    switch (x->e_modenum)
    {
        case 0: easing_quadIn(x);
            break;
        case 1: easing_quadOut(x);
            break;
        case 2: easing_quadInOut(x);
            break;
        case 10: easing_cubicIn(x);
            break;
        case 11: easing_cubicOut(x);
            break;
        case 12: easing_cubicInOut(x);
            break;
        case 20: easing_quartIn(x);
            break;
        case 21: easing_quartOut(x);
            break;
        case 22: easing_quartInOut(x);
            break;
        case 30: easing_quintIn(x);
            break;
        case 31: easing_quintOut(x);
            break;
        case 32: easing_quintInOut(x);
            break;
        case 40: easing_sinIn(x);
            break;
        case 41: easing_sinOut(x);
            break;
        case 42: easing_sinInOut(x);
            break;
        case 50: easing_expIn(x);
            break;
        case 51: easing_expOut(x);
            break;
        case 52: easing_expInOut(x);
            break;
        case 60: easing_circIn(x);
            break;
        case 61: easing_circOut(x);
            break;
        case 62: easing_circInOut(x);
            break;
    }
}

/*-----clock function----------------------------------------*/

void metro_start(t_easing *x)
{
    x->e_count = 0 - x->e_interval;
    clock_delay(x->e_clock, 0L);  // set clock to go off now
}

void clock_function(t_easing *x)
{
    clock_delay(x->e_clock, x->e_interval); // schedule another metronome click
    
    x->e_t = x->e_count += x->e_interval; // counter
    easing_culc(x);
    outlet_float(x->e_out, x->e_b);
    
    if(x->e_count >= x->e_d)
    {
        metro_stop(x);
        outlet_bang(x->e_bangout);
    }
}

void easing_stop(t_easing *x)
{
    metro_stop(x);
}

void metro_stop(t_easing *x)
{
    clock_unset(x->e_clock); // remove the clock routine from the scheduler
}

void easing_print(t_easing *x)
{
    post("t: %f", x->e_t);
    post("bf: %f", x->e_b);
    post("dest: %f", x->e_dest);
    post("d: %f", x->e_d);
    post("mode: %i", x->e_modenum);
    post("interval: %f", x->e_interval);
}

/*-----culculation functions---------------------------------*/

void easing_quadIn(t_easing *x)
{
    x->e_b = x->e_c*(x->e_t/=x->e_d)*x->e_t+x->e_current;
}

void easing_quadOut(t_easing *x)
{
    x->e_b = -x->e_c*(x->e_t/=x->e_d)*(x->e_t-2)+x->e_current;
}

void easing_quadInOut(t_easing *x)
{
    if((x->e_t/=x->e_d/2) < 1)
        x->e_b = x->e_c/2*x->e_t*x->e_t+x->e_current;
    else
        x->e_b = -x->e_c/2*(--x->e_t*(x->e_t-2)-1)+x->e_current;
}

void easing_cubicIn(t_easing *x)
{
    x->e_b = x->e_c*pow(x->e_t/x->e_d, 3)+x->e_current;
}

void easing_cubicOut(t_easing *x)
{
    x->e_b = x->e_c*(pow(x->e_t/x->e_d-1, 3)+1)+x->e_current;
}

void easing_cubicInOut(t_easing *x)
{
    if((x->e_t/=x->e_d/2) < 1)
        x->e_b = x->e_c/2*pow(x->e_t, 3)+x->e_current;
    else
        x->e_b = x->e_c/2*(pow(x->e_t-2, 3)+2)+x->e_current;
}

void easing_quartIn(t_easing *x)
{
    x->e_b = x->e_c*pow(x->e_t/x->e_d, 4)+x->e_current;
}

void easing_quartOut(t_easing *x)
{
    x->e_b = -x->e_c*(pow(x->e_t/x->e_d-1, 4)-1)+x->e_current;
}

void easing_quartInOut(t_easing *x)
{
    if((x->e_t/=x->e_d/2) < 1)
        x->e_b = x->e_c/2*pow(x->e_t, 4)+x->e_current;
    else
        x->e_b = -x->e_c/2*(pow(x->e_t-2, 4)-2)+x->e_current;
}

void easing_quintIn(t_easing *x)
{
    x->e_b = x->e_c*pow(x->e_t/x->e_d, 5)+x->e_current;
}

void easing_quintOut(t_easing *x)
{
    x->e_b = x->e_c*(pow(x->e_t/x->e_d-1, 5)+1)+x->e_current;
}

void easing_quintInOut(t_easing *x)
{
    if((x->e_t/=x->e_d/2) < 1)
        x->e_b = x->e_c/2*pow(x->e_t, 5)+x->e_current;
    else
        x->e_b = x->e_c/2*(pow(x->e_t-2, 5)+2)+x->e_current;
}

void easing_sinIn(t_easing *x)
{
    x->e_b = x->e_c*(1-cos(x->e_t/x->e_d*(PI/2)))+x->e_current;
}

void easing_sinOut(t_easing *x)
{
    x->e_b = x->e_c*sin(x->e_t/x->e_d*(PI/2))+x->e_current;
}

void easing_sinInOut(t_easing *x)
{
    x->e_b = x->e_c/2*(1-cos(PI*x->e_t/x->e_d))+x->e_current;
}

void easing_expIn(t_easing *x)
{
    x->e_b = x->e_c*pow(2, 10*(x->e_t/x->e_d-1))+x->e_current;
}

void easing_expOut(t_easing *x)
{
    double temp1 = x->e_t/x->e_d;
    double temp2 = pow(2, -10*temp1);
    x->e_b = x->e_c*(-temp2+1+(temp1*temp2))+x->e_current;
}

void easing_expInOut(t_easing *x)
{
    double temp1 = x->e_t/x->e_d;
    double temp2 = pow(2, -10*temp1);
    if((x->e_t/=x->e_d/2) < 1)
        x->e_b = x->e_c/2*pow(2, 10*(x->e_t-1))+x->e_current;
    else
        x->e_b = x->e_c/2*((-pow(2, -10*(x->e_t-1))+2+(temp1*temp2)))+x->e_current;
}

void easing_circIn(t_easing *x)
{
    x->e_b = x->e_c*(1-sqrt(1-(x->e_t/=x->e_d)*x->e_t))+x->e_current;
}

void easing_circOut(t_easing *x)
{
    x->e_b = x->e_c*sqrt(1-(x->e_t=x->e_t/x->e_d-1)*x->e_t)+x->e_current;
}

void easing_circInOut(t_easing *x)
{
    if((x->e_t/=x->e_d/2) < 1)
        x->e_b = x->e_c/2*(1-sqrt(1-x->e_t*x->e_t))+x->e_current;
    else
        x->e_b = x->e_c/2*(sqrt(1-(x->e_t-=2)*x->e_t)+1)+x->e_current;
}

/*
 Terms of Use: Easing Functions (Equations)
 
 Open source under the MIT License and the 3-Clause BSD License.
 
 MIT License
 
 Copyright © 2001 Robert Penner
 
 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 BSD License
 
 Copyright © 2001 Robert Penner
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 Neither the name of the author nor the names of contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

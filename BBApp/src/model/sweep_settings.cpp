#include "sweep_settings.h"

#include <QSettings>

#include "../lib/bb_api.h"

double SweepSettings::maxRealTimeSpan = BB60A_MAX_RT_SPAN;

SweepSettings::SweepSettings()
{
    LoadDefaults();    
}

SweepSettings::SweepSettings(const SweepSettings &other)
{
    *this = other;
}

SweepSettings& SweepSettings::operator=(const SweepSettings &other)
{
    mode = other.mode;

    start = other.start;
    stop = other.stop;
    center = other.center;
    span = other.span;
    step = other.step;
    rbw = other.rbw;
    vbw = other.vbw;

    auto_rbw = other.auto_rbw;
    auto_vbw = other.auto_vbw;
    native_rbw = other.native_rbw;

    refLevel = other.refLevel;
    div = other.div;
    attenuation = other.attenuation;
    gain = other.gain;

    sweepTime = other.sweepTime;
    processingUnits = other.processingUnits;
    detector = other.detector;
    rejection = other.rejection;

    // Assuming this is needed for now ?
    emit updated(this);

    return *this;
}

bool SweepSettings::operator==(const SweepSettings &other) const
{
    if(mode != other.mode) return false;

    if(start != other.start) return false;
    if(stop != other.stop) return false;
    if(center != other.center) return false;
    if(span != other.span) return false;
    if(step != other.step) return false;
    if(rbw != other.rbw) return false;
    if(vbw != other.vbw) return false;

    if(auto_rbw != other.auto_rbw) return false;
    if(auto_vbw != other.auto_vbw) return false;

    if(refLevel != other.refLevel) return false;
    if(div != other.div) return false;
    if(attenuation != other.attenuation) return false;
    if(gain != other.gain) return false;

    if(sweepTime != other.sweepTime) return false;
    if(processingUnits != other.processingUnits) return false;
    if(detector != other.detector) return false;
    if(rejection != other.rejection) return false;

    return true;
}

bool SweepSettings::operator!=(const SweepSettings &other) const
{
    return !(*this == other);
}

// Default values, program launch
void SweepSettings::LoadDefaults()
{
    mode = MODE_SWEEPING;

    start = 11.0e6;
    stop = 6.0e9;
    span = (stop - start);
    center = (start + stop) / 2.0;
    step = 20.0e6;
    rbw = 300.0e3;
    vbw = 300.0e3;

    auto_rbw = true;
    auto_vbw = true;
    native_rbw = false;

    refLevel = Amplitude(-30.0, DBM);
    div = 10.0;
    attenuation = 0;
    gain = 0;

    // Standard sweep only, real-time sweep time in prefs
    sweepTime = 0.001;
    processingUnits = BB_POWER;
    detector = BB_AVERAGE;
    rejection = false;

    //emit updated(this);
}

// Preset Load
bool SweepSettings::Load(QSettings &s)
{
    mode = (OperationalMode)(s.value("Mode", (int)Mode()).toInt());

    start = s.value("Sweep/Start", Start().Val()).toDouble();
    stop = s.value("Sweep/Stop", Stop().Val()).toDouble();
    center = s.value("Sweep/Center", Center().Val()).toDouble();
    span = s.value("Sweep/Span", Span().Val()).toDouble();
    step = s.value("Sweep/Step", Step().Val()).toDouble();
    rbw = s.value("Sweep/RBW", RBW().Val()).toDouble();
    vbw = s.value("Sweep/VBW", VBW().Val()).toDouble();

    auto_rbw = s.value("Sweep/AutoRBW", AutoRBW()).toBool();
    auto_vbw = s.value("Sweep/AutoVBW", AutoVBW()).toBool();
    native_rbw = s.value("Sweep/NativeRBW", NativeRBW()).toBool();

    refLevel = s.value("Sweep/RefLevel", RefLevel().Val()).toDouble();
    div = s.value("Sweep/Division", Div()).toDouble();
    attenuation = s.value("Sweep/Attenuation", Attenuation()).toInt();
    gain = s.value("Sweep/Gain", Gain()).toInt();

    sweepTime = s.value("Sweep/SweepTime", SweepTime().Val()).toDouble();
    processingUnits = s.value("Sweep/ProcessingUnits", ProcessingUnits()).toInt();
    detector = s.value("Sweep/Detector", Detector()).toInt();
    rejection = s.value("Sweep/Rejection", Rejection()).toBool();

    emit updated(this);
    return true;
}

bool SweepSettings::Save(QSettings &s) const
{
    s.setValue("Mode", mode);

    s.setValue("Sweep/Start", start.Val());
    s.setValue("Sweep/Stop", stop.Val());
    s.setValue("Sweep/Center", center.Val());
    s.setValue("Sweep/Span", span.Val());
    s.setValue("Sweep/Step", step.Val());
    s.setValue("Sweep/RBW", rbw.Val());
    s.setValue("Sweep/VBW", vbw.Val());

    s.setValue("Sweep/AutoRBW", auto_rbw);
    s.setValue("Sweep/AutoVBW", auto_vbw);
    s.setValue("Sweep/NativeRBW", native_rbw);

    s.setValue("Sweep/RefLevel", refLevel.Val());
    s.setValue("Sweep/Division", div);
    s.setValue("Sweep/Attenuation", attenuation);
    s.setValue("Sweep/Gain", gain);

    s.setValue("Sweep/SweepTime", sweepTime.Val());
    s.setValue("Sweep/ProcessingUnits", processingUnits);
    s.setValue("Sweep/Detector", detector);
    s.setValue("Sweep/Rejection", rejection);

    return true;
}

bool SweepSettings::IsAveragePower() const
{
    return (Detector() == BB_AVERAGE &&
            ProcessingUnits() == BB_POWER);
}

void SweepSettings::AutoBandwidthAdjust(bool force)
{    
    if(Mode() == BB_REAL_TIME) {
        native_rbw = true;
    }

    if(auto_rbw || force) {
        rbw = bb_lib::get_best_rbw(span, native_rbw);
    } else {
        bb_lib::adjust_rbw_on_span(rbw, span, native_rbw);
    }

    if(auto_vbw || vbw > rbw || mode == BB_REAL_TIME) {
        vbw = rbw;
    }

    if(mode == BB_REAL_TIME) {
        double clamped = rbw;
        bb_lib::clamp(clamped, BB_MIN_RT_RBW, BB_MAX_RT_RBW);
        rbw = clamped;
        vbw = rbw;
    }
}

void SweepSettings::setMode(OperationalMode new_mode)
{
    mode = new_mode;

    if(mode == BB_REAL_TIME) {
        native_rbw = true;
        auto_rbw = true;
        auto_vbw = true;
        if(span > maxRealTimeSpan) {
            span = maxRealTimeSpan;
            start = center - (maxRealTimeSpan / 2.0);
            stop = center + (maxRealTimeSpan / 2.0);
        }

        AutoBandwidthAdjust(true);
        //emit updated(this);
    }
}

/*
 * Update start without changing stop, otherwise do nothing
 */
void SweepSettings::setStart(Frequency f)
{   
    bool valid = false;
    Frequency min_start = bb_lib::max2(f.Val(), BB60_MIN_FREQ);

    // Only change if room
    if(min_start < (stop - BB_MIN_SPAN)) {
        // Special case in real-time
        if(mode == MODE_REAL_TIME) {
            if((stop - min_start) <= maxRealTimeSpan &&
                    (stop - min_start) >= BB_MIN_RT_SPAN) {
                valid = true;
            }
        } else { // Other mode
            valid = true;
        }
    }

    if(valid) {
        start = min_start;
        span = stop - start;
        center = start + (span / 2.0);
        //AdjustForSweepSize();
    }

    AutoBandwidthAdjust(false);
    emit updated(this);
}

/*
 * Update stop without changing start, otherwise to nothing
 */
void SweepSettings::setStop(Frequency f)
{
    bool valid = false;
    Frequency max_stop = bb_lib::min2(f.Val(), BB60_MAX_FREQ);

    // Only change if room
    if(max_stop > (start + BB_MIN_SPAN)) {
        if(mode == MODE_REAL_TIME) {
            if((max_stop - start) <= maxRealTimeSpan &&
                    (max_stop - start) >= BB_MIN_RT_SPAN) {
                valid = true;
            }
        } else { // other mode
            valid = true;
        }
    }

    if(valid) {
        stop = max_stop;
        span = stop - start;
        center = start + (span / 2.0);
}

    AutoBandwidthAdjust(false);
    emit updated(this);
}

void SweepSettings::setCenter(Frequency f)
{
    // Is the center even possible?
    if(f < (BB60_MIN_FREQ + BB_MIN_SPAN * 2.0) ||
            f > (BB60_MAX_FREQ - BB_MIN_SPAN * 2.0)) {
        // Do nothing
    } else {
        center = f;
        span = bb_lib::min3(span.Val(),
                            (center - BB60_MIN_FREQ) * 2.0,
                            (BB60_MAX_FREQ - center) * 2.0);
        start = center - span / 2.0;
        stop = center + span / 2.0;
    }

    AutoBandwidthAdjust(false);
    emit updated(this);
}

void SweepSettings::increaseCenter(bool inc)
{
    if(inc) {
        setCenter(center + step);
    } else {
        setCenter(center - step);
    }
}

void SweepSettings::setSpan(Frequency f)
{
    if(f < BB_MIN_SPAN) {
        f = BB_MIN_SPAN;
    }

    if(Mode() == MODE_REAL_TIME || Mode() == MODE_TIME_GATE) {
        bb_lib::clamp(f, Frequency(BB_MIN_RT_SPAN), Frequency(maxRealTimeSpan));
    }

    // Fit new span to device freq range
    if((center - f / 2.0) < BB60_MIN_FREQ) {
        start = BB60_MIN_FREQ;
        stop = bb_lib::min2((start + f).Val(), BB60_MAX_FREQ);
    } else if((center + f / 2.0) > BB60_MAX_FREQ) {
        stop = BB60_MAX_FREQ;
        start = bb_lib::max2((stop - f).Val(), BB60_MIN_FREQ);
    } else {
        start = center - f / 2.0;
        stop = center + f / 2.0;
    }

    center = (start + stop) / 2.0;
    span = stop - start;

    AutoBandwidthAdjust(false);
    emit updated(this);
}

void SweepSettings::increaseSpan(bool inc)
{
    double new_span = bb_lib::sequence_span(span, inc);
    setSpan(new_span);
}

void SweepSettings::setStep(Frequency f)
{
    step = f;

    emit updated(this);
}

void SweepSettings::setFullSpan()
{
    start = 10.0e6;
    stop = 6.0e9;
    center = (stop + start) / 2.0;
    span = stop - start;

    auto_rbw = true;
    auto_vbw = true;

    AutoBandwidthAdjust(false);
    emit updated(this);
}

void SweepSettings::setRBW(Frequency f)
{
    if(native_rbw) {
        int ix = bb_lib::get_native_bw_index(f);
        rbw = native_bw_lut[ix].bw;
    } else {
        rbw = f;
    }

    auto_rbw = false;
    AutoBandwidthAdjust(false);

//    if(vbw > rbw || auto_vbw) {
//        vbw = rbw;
//    }

    emit updated(this);
}

void SweepSettings::setVBW(Frequency f)
{
    vbw = f;

    if(vbw > rbw) {
        vbw = rbw;
    }

    auto_vbw = false;
    AutoBandwidthAdjust(false);
    emit updated(this);
}

void SweepSettings::rbwIncrease(bool inc)
{
    double new_rbw = bb_lib::sequence_bw(rbw, native_rbw, inc);

    rbw = new_rbw;
    auto_rbw = false;
    AutoBandwidthAdjust(false);

//    if(auto_vbw) vbw = rbw;
//    if(vbw > rbw) vbw = rbw;

    emit updated(this);
}

void SweepSettings::vbwIncrease(bool inc)
{
    double new_vbw = bb_lib::sequence_bw(vbw, native_rbw, inc);

    if(new_vbw > rbw) new_vbw = rbw;

    auto_vbw = false;
    vbw = new_vbw;

    AutoBandwidthAdjust(false);
    emit updated(this);
}

void SweepSettings::setAutoRbw(bool new_auto)
{
    auto_rbw = new_auto;

    AutoBandwidthAdjust(false);

    emit updated(this);
}

void SweepSettings::setAutoVbw(bool new_auto)
{
    auto_vbw = new_auto;

    if(auto_vbw) {
        vbw = rbw;
    }

    emit updated(this);
}

void SweepSettings::setNativeRBW(bool native)
{
    native_rbw = native;
    auto_rbw = true;

    AutoBandwidthAdjust(true);

    emit updated(this);
}

void SweepSettings::setRefLevel(Amplitude new_ref)
{
    new_ref.Clamp(Amplitude(-100, DBM), Amplitude(20.0, DBM));

    refLevel = new_ref;
    emit updated(this);
}

void SweepSettings::shiftRefLevel(bool inc)
{
    if(refLevel.IsLogScale()) {
        if(inc) refLevel += div;
        else refLevel -= div;
    } else {
        if(inc) refLevel = Amplitude(refLevel.Val() * 1.2, AmpUnits::MV);
        else refLevel = Amplitude(refLevel.Val() * 0.8, AmpUnits::MV);
    }

    emit updated(this);
}

void SweepSettings::setDiv(double new_div)
{
    bb_lib::clamp(new_div, 0.1, 30.0);
    div = new_div;
    emit updated(this);
}

/*
 * Store just the index
 * Convert to real atten in device->configure
 */
void SweepSettings::setAttenuation(int atten_ix)
{
    attenuation = atten_ix;
    emit updated(this);
}

/*
 * Store just the index
 * Convert to real gain in device->configure
 */
void SweepSettings::setGain(int gain_ix)
{
    gain = gain_ix;
    emit updated(this);
}

void SweepSettings::setDetector(int new_detector)
{
    if(detector != new_detector) {
        detector = new_detector;
        emit updated(this);
    }
}

void SweepSettings::setProcUnits(int new_units)
{
    if(processingUnits != new_units) {
        processingUnits = new_units;
        emit updated(this);
    }
}

void SweepSettings::setSweepTime(Time new_sweep_time)
{    
    sweepTime = new_sweep_time;

    emit updated(this);
}

void SweepSettings::setRejection(bool image_reject)
{
    if(rejection != image_reject) {
        rejection = image_reject;
        emit updated(this);
    }
}

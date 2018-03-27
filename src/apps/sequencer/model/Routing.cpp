#include "Routing.h"

#include "Project.h"

//----------------------------------------
// Routing::MidiSource
//----------------------------------------

void Routing::MidiSource::clear() {
    _port = Types::MidiPort::Midi;
    _channel = -1;
    _event = Event::ControllerAbs;
    _controllerOrNote = 0;
}

void Routing::MidiSource::write(WriteContext &context) const {
    auto &writer = context.writer;
    writer.write(_port);
    writer.write(_channel);
    writer.write(_event);
    writer.write(_controllerOrNote);
}

void Routing::MidiSource::read(ReadContext &context) {
    auto &reader = context.reader;
    reader.read(_port);
    reader.read(_channel);
    reader.read(_event);
    reader.read(_controllerOrNote);
}

bool Routing::MidiSource::operator==(const MidiSource &other) const {
    return (
        _port == other._port &&
        _channel == other._channel &&
        _event == other._event &&
        _controllerOrNote == other._controllerOrNote
    );
}

//----------------------------------------
// Routing::Route
//----------------------------------------

void Routing::Route::clear() {
    _param = Param::None;
    _track = -1;
    _min = 0.f;
    _max = 1.f;
    _source = Source::None;
    _midiSource.clear();
}

void Routing::Route::init(Param param, int track) {
    clear();
    _param = param;
}

void Routing::Route::write(WriteContext &context) const {
    auto &writer = context.writer;
    writer.write(_param);
    writer.write(_track);
    writer.write(_min);
    writer.write(_max);
    writer.write(_source);
    if (_source == Source::Midi) {
        _midiSource.write(context);
    }
}

void Routing::Route::read(ReadContext &context) {
    auto &reader = context.reader;
    reader.read(_param);
    reader.read(_track);
    reader.read(_min);
    reader.read(_max);
    reader.read(_source);
    if (_source == Source::Midi) {
        _midiSource.read(context);
    }
}

bool Routing::Route::operator==(const Route &other) const {
    return (
        _param == other._param &&
        _track == other._track &&
        _min == other._min &&
        _max == other._max &&
        _source == other._source &&
        _midiSource == other._midiSource
    );
}

//----------------------------------------
// Routing
//----------------------------------------

Routing::Routing(Project &project) :
    _project(project)
{}

void Routing::clear() {
    for (auto &route : _routes) {
        route.clear();
    }

    // {
    //     auto route = addRoute(Param::BPM);
    //     route->source().initMIDI();
    //     // route->source().midi().setKind(MIDISource::Kind::ControllerAbs);
    //     // route->source().midi().setPort(MIDISource::Port::MIDI);
    //     // route->source().midi().setChannel(1);
    //     // route->source().midi().setController(16);

    //     // route->source().midi().setKind(MIDISource::Kind::PitchBend);
    //     // route->source().midi().setPort(MIDISource::Port::MIDI);
    //     // route->source().midi().setChannel(1);

    //     route->source().midi().setKind(MIDISource::Kind::NoteVelocity);
    //     route->source().midi().setPort(MIDISource::Port::MIDI);
    //     route->source().midi().setChannel(1);
    //     route->source().midi().setNote(60);

    // }

    // {
    //     auto route = addRoute(Param::Swing);
    //     route->source().initMIDI();
    //     route->source().midi().setKind(MIDISource::Kind::ControllerRel);
    //     route->source().midi().setPort(MIDISource::Port::MIDI);
    //     route->source().midi().setChannel(1);
    //     route->source().midi().setController(63);
    // }
}

Routing::Route *Routing::nextFreeRoute() {
    for (auto &route : _routes) {
        if (!route.active()) {
            return &route;
        }
    }
    return nullptr;
}

const Routing::Route *Routing::findRoute(Param param, int trackIndex) const {
    for (auto &route : _routes) {
        if (route.active() && route.param() == param && route.track() == trackIndex) {
            return &route;
        }
    }
    return nullptr;
}

Routing::Route *Routing::findRoute(Param param, int trackIndex) {
    for (auto &route : _routes) {
        if (route.active() && route.param() == param && route.track() == trackIndex) {
            return &route;
        }
    }
    return nullptr;
}

Routing::Route *Routing::addRoute(Param param, int trackIndex) {
    Route *route = findRoute(param, trackIndex);
    if (route) {
        return route;
    }

    route = nextFreeRoute();
    if (!route) {
        return nullptr;
    }

    route->init(param, trackIndex);

    return route;
}

void Routing::removeRoute(Route *route) {
    if (route) {
        route->clear();
    }
}

void Routing::writeParam(Param param, int trackIndex, int patternIndex, float value) {
    value = denormalizeParamValue(param, value);
    switch (param) {
    case Param::BPM:
        _project.setBpm(value);
        break;
    case Param::Swing:
        _project.setSwing(value);
        break;
    default:
        writeTrackParam(param, trackIndex, patternIndex, value);
        break;
    }
}

void Routing::writeTrackParam(Param param, int trackIndex, int patternIndex, float value) {
    auto &track = _project.track(trackIndex);
    switch (track.trackMode()) {
    case Track::TrackMode::Note:
        writeNoteSequenceParam(track.noteTrack().sequence(patternIndex), param, value);
        break;
    case Track::TrackMode::Curve:
        writeCurveSequenceParam(track.curveTrack().sequence(patternIndex), param, value);
        break;
    case Track::TrackMode::MidiCv:
        // TODO
        break;
    case Track::TrackMode::Last:
        break;
    }
}

void Routing::writeNoteSequenceParam(NoteSequence &sequence, Param param, float value) {
    switch (param) {
    case Param::FirstStep:
        sequence.setFirstStep(value);
        break;
    case Param::LastStep:
        sequence.setLastStep(value);
        break;
    default:
        break;
    }
}

void Routing::writeCurveSequenceParam(CurveSequence &sequence, Param param, float value) {

}

float Routing::readParam(Param param, int patternIndex, int trackIndex) const {
    switch (param) {
    case Param::BPM:
        return _project.bpm();
    case Param::Swing:
        return _project.swing();
    default:
        return 0.f;
    }
}

void Routing::write(WriteContext &context) const {
    writeArray(context, _routes);
}

void Routing::read(ReadContext &context) {
    readArray(context, _routes);
}




struct ParamInfo {
    int16_t min;
    int16_t max;
};

const ParamInfo paramInfos[int(Routing::Param::Last)] = {
    [int(Routing::Param::None)]             = { 0,      0   },
    [int(Routing::Param::BPM)]              = { 20,     500 },
    [int(Routing::Param::Swing)]            = { 50,     75  },
    [int(Routing::Param::TrackTranspose)]   = { -12,    12  },
    [int(Routing::Param::TrackRotate)]      = { -64,    64  },
    [int(Routing::Param::FirstStep)]        = { 0,      63  },
    [int(Routing::Param::LastStep)]         = { 0,      63  },
};

float Routing::normalizeParamValue(Routing::Param param, float value) {
    const auto &info = paramInfos[int(param)];
    return clamp((value - info.min) / (info.max - info.min), 0.f, 1.f);
}

float Routing::denormalizeParamValue(Routing::Param param, float normalized) {
    const auto &info = paramInfos[int(param)];
    return normalized * (info.max - info.min) + info.min;
}

float Routing::paramValueStep(Routing::Param param) {
    const auto &info = paramInfos[int(param)];
    return 1.f / (info.max - info.min);
}

void Routing::printParamValue(Routing::Param param, float normalized, StringBuilder &str) {
    float value = denormalizeParamValue(param, normalized);
    switch (param) {
    case Param::None:
        str("-");
        break;
    case Param::BPM:
        str("%.1f", value);
        break;
    case Param::Swing:
        str("%d%%", int(value));
        break;
    case Param::TrackTranspose:
    case Param::TrackRotate:
        str("%+d", int(value));
        break;
    case Param::FirstStep:
    case Param::LastStep:
        str("%d", int(value) + 1);
        break;
    default:
        str("%d", int(value));
        break;
    }
}

#pragma once
#include <Preferences.h>
#include "stations.h"

#define _ST_NS      "radio_st"
#define _ST_MAX     30
#define _ST_NLEN    32
#define _ST_ULEN    200
#define _DEL_BYTES  ((STATION_COUNT + 7) / 8)

struct StationEntry {
    char name[_ST_NLEN];
    char lang[8];
    char url[_ST_ULEN];
    bool isBuiltin;
    int  sourceIdx;   // built-in index OR custom index
};

static uint8_t _delMask[_DEL_BYTES] = {};
static int     _stCount = -1;   // -1 = not loaded yet
static char    _stN[_ST_MAX][_ST_NLEN];
static char    _stU[_ST_MAX][_ST_ULEN];

static bool _isDeleted(int bi) {
    return (_delMask[bi / 8] >> (bi % 8)) & 1;
}
static void _setDeleted(int bi, bool del) {
    if (del) _delMask[bi/8] |=  (1u << (bi%8));
    else     _delMask[bi/8] &= ~(1u << (bi%8));
}
static void _saveMask() {
    Preferences p; p.begin(_ST_NS, false);
    p.putBytes("delmask", _delMask, _DEL_BYTES);
    p.end();
}
static void _saveCustom() {
    Preferences p; p.begin(_ST_NS, false);
    p.putInt("count", _stCount);
    for (int i = 0; i < _stCount; i++) {
        p.putString(("n" + String(i)).c_str(), _stN[i]);
        p.putString(("u" + String(i)).c_str(), _stU[i]);
    }
    p.end();
}

static void storeLoad() {
    Preferences p; p.begin(_ST_NS, true);
    p.getBytes("delmask", _delMask, _DEL_BYTES);
    _stCount = (int)p.getInt("count", 0);
    if (_stCount > _ST_MAX) _stCount = _ST_MAX;
    for (int i = 0; i < _stCount; i++) {
        String v = p.getString(("n" + String(i)).c_str(), "");
        strncpy(_stN[i], v.c_str(), _ST_NLEN-1); _stN[i][_ST_NLEN-1]='\0';
        v = p.getString(("u" + String(i)).c_str(), "");
        strncpy(_stU[i], v.c_str(), _ST_ULEN-1); _stU[i][_ST_ULEN-1]='\0';
    }
    p.end();
}

static int getTotalStationCount() {
    if (_stCount < 0) storeLoad();
    int n = 0;
    for (int i = 0; i < STATION_COUNT; i++) if (!_isDeleted(i)) n++;
    return n + _stCount;
}

static int getCustomStationCount() {
    if (_stCount < 0) storeLoad();
    return _stCount;
}

// playerIdx — index in the "active" list (non-deleted built-ins, then custom)
static StationEntry getStation(int playerIdx) {
    if (_stCount < 0) storeLoad();
    StationEntry e = {};
    for (int i = 0; i < STATION_COUNT; i++) {
        if (_isDeleted(i)) continue;
        if (playerIdx-- == 0) {
            strncpy(e.name, stations[i].name, sizeof(e.name)-1);
            strncpy(e.lang, stations[i].lang, sizeof(e.lang)-1);
            strncpy(e.url,  stations[i].url,  sizeof(e.url)-1);
            e.isBuiltin = true;
            e.sourceIdx = i;
            return e;
        }
    }
    int ci = playerIdx;   // playerIdx was decremented past all active built-ins
    if (ci >= 0 && ci < _stCount) {
        strncpy(e.name, _stN[ci], sizeof(e.name)-1);
        strncpy(e.url,  _stU[ci], sizeof(e.url)-1);
        strncpy(e.lang, "MY",     sizeof(e.lang)-1);
        e.isBuiltin = false;
        e.sourceIdx = ci;
    }
    return e;
}

static bool storeAdd(const char *name, const char *url) {
    if (_stCount < 0) storeLoad();
    if (_stCount >= _ST_MAX) return false;
    strncpy(_stN[_stCount], name, _ST_NLEN-1);
    strncpy(_stU[_stCount], url,  _ST_ULEN-1);
    _stCount++;
    _saveCustom();
    return true;
}

// Delete by player index. Returns true on success.
static bool storeDeleteByIdx(int playerIdx) {
    if (_stCount < 0) storeLoad();
    for (int i = 0; i < STATION_COUNT; i++) {
        if (_isDeleted(i)) continue;
        if (playerIdx-- == 0) {
            _setDeleted(i, true);
            _saveMask();
            return true;
        }
    }
    int ci = playerIdx;
    if (ci < 0 || ci >= _stCount) return false;
    for (int i = ci; i < _stCount-1; i++) {
        strcpy(_stN[i], _stN[i+1]);
        strcpy(_stU[i], _stU[i+1]);
    }
    _stCount--;
    _saveCustom();
    return true;
}

static void storeRestoreBuiltins() {
    memset(_delMask, 0, sizeof(_delMask));
    _saveMask();
}

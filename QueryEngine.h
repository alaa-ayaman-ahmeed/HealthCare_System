#ifndef QUERY_ENGINE_H
#define QUERY_ENGINE_H

#include <iostream>
#include <string>
#include <vector>

#include "StorageManager.h"
#include "PrimaryIndex.h"
#include "SecondaryIndex.h"

using namespace std;

class QueryEngine {

private:

    StorageManager& storage;
    PrimaryIndex& primary;

    SecondaryIndex& docNameIdx;
    SecondaryIndex& docSpecIdx;
    SecondaryIndex& appDocIdx;

    // Helpers
    string cleanField(const char* data, int size) const;

    string toUpper(string text) const;

    string trim(string text) const;

    vector<string> splitConditions(const string& wherePart) const;

    bool parseCondition(
        const string& condition,
        string& field,
        string& value
    ) const;

    vector<string> intersect(
        const vector<string>& first,
        const vector<string>& second
    ) const;

    bool getDoctorRecord(
        const string& id,
        DoctorRecord& doctor
    ) const;

    bool getAppointmentRecord(
        const string& id,
        AppointmentRecord& appointment
    ) const;

    void printDoctor(
        const DoctorRecord& doctor,
        const string& selectedFields
    ) const;

    void printAppointment(
        const AppointmentRecord& appointment,
        const string& selectedFields
    ) const;

public:

    QueryEngine(
        StorageManager& sm,
        PrimaryIndex& pi,
        SecondaryIndex& nameIdx,
        SecondaryIndex& specIdx,
        SecondaryIndex& appIdx
    );

    bool executeQuery(const string& query);
};

#endif
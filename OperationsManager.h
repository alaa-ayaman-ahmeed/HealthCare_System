
#ifndef OPERATIONS_MANAGER_H
#define OPERATIONS_MANAGER_H

#include "StorageManager.h"
#include "PrimaryIndex.h"
#include "SecondaryIndex.h"

#include <string>
using namespace std;

class OperationsManager {

private:

    StorageManager& storage;
    PrimaryIndex& primary;

    SecondaryIndex& docNameIdx;
    SecondaryIndex& docSpecIdx;
    SecondaryIndex& appDocIdx;

    // Helpers
    int getFileEnd(const string& filename);

    int getDoctorOffset(const string& id);
    int getAppointmentOffset(const string& id);

    string cleanField(const char* data, int size);

    bool validDoctor(const string& id,
                     const string& name,
                     const string& specialty,
                     const string& address);

    bool validAppointment(const string& id,
                          const string& date,
                          const string& doctorID);

public:

    OperationsManager(
        StorageManager& sm,
        PrimaryIndex& pi,
        SecondaryIndex& nameIdx,
        SecondaryIndex& specIdx,
        SecondaryIndex& appIdx
    );

    // ADD
    bool addDoctor(const string& id,
                   const string& name,
                   const string& specialty,
                   const string& address);

    bool addAppointment(const string& id,
                        const string& date,
                        const string& doctorID);

    // DELETE
    bool deleteDoctor(const string& id);

    bool deleteAppointment(const string& id);

    // UPDATE
    bool updateDoctorName(const string& id,
                          const string& newName);

    bool updateDoctorID(const string& oldID,
                        const string& newID);

    bool updateAppointmentDate(const string& id,
                               const string& newDate);
};

#endif

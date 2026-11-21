
#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <string>
using namespace std;

const int HEADER_SIZE = 4;
const int DOCTOR_RECORD_SIZE = 95;
const int APPOINTMENT_RECORD_SIZE = 60;

struct DoctorRecord {
    char doctorID[15];
    char doctorName[30];
    char doctorSpecialty[20];
    char address[30];
};

struct AppointmentRecord {
    char appointmentID[15];
    char appointmentDate[30];
    char doctorID[15];
};

class StorageManager {

private:
    void padString(const string& source, char* dest, int exactSize);

public:
    void initializeFiles();

    int readHeader(const string& filename);

    bool writeHeader(const string& filename, int newHeaderVal);

    bool writeAt(
        const string& filename,
        int offset,
        const char* buffer,
        int size
    );

    bool readAt(
        const string& filename,
        int offset,
        char* buffer,
        int size
    );

    void saveDoctor(
        const string& id,
        const string& name,
        const string& specialty,
        const string& address,
        char buffer[DOCTOR_RECORD_SIZE]
    );

    void saveAppointment(
        const string& appId,
        const string& appDate,
        const string& docId,
        char buffer[APPOINTMENT_RECORD_SIZE]
    );

    bool writeDoctorRecord(
        int recordNumber,
        const char buffer[DOCTOR_RECORD_SIZE]
    );

    bool writeAppointmentRecord(
        int recordNumber,
        const char buffer[APPOINTMENT_RECORD_SIZE]
    );
};

#endif

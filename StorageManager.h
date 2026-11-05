#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <string>
using namespace std;

// File sizes
const int HEADER_SIZE = 4;
const int DOCTOR_RECORD_SIZE = 95;
const int APPOINTMENT_RECORD_SIZE = 60;

// Doctor record fields: 15 + 30 + 20 + 30 = 95 bytes
struct DoctorRecord {
    char doctorID[15];
    char doctorName[30];
    char doctorSpecialty[20];
    char address[30];
};

// Appointment record fields: 15 + 30 + 15 = 60 bytes
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

    void writeHeader(const string& filename, int newHeaderVal);

    void writeAt(const string& filename, int offset,const char* buffer, int size);

    void readAt(const string& filename, int offset,char* buffer, int size);

    void saveDoctor(const string& id, const string& name,const string& specialty, const string& address,char buffer[DOCTOR_RECORD_SIZE]);

    void saveAppointment(const string& appId, const string& appDate,const string& docId,char buffer[APPOINTMENT_RECORD_SIZE]);

    void writeDoctorRecord(int recordNumber,const char buffer[DOCTOR_RECORD_SIZE]);

    void writeAppointmentRecord(int recordNumber,const char buffer[APPOINTMENT_RECORD_SIZE]);
};

#endif 
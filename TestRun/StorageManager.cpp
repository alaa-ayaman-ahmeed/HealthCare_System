
#include "StorageManager.h"

#include <fstream>
#include <cstring>
#include <algorithm>
#include <climits>

using namespace std;

void StorageManager::padString(
    const string& source,
    char* dest,
    int exactSize
) {
    memset(dest, ' ', exactSize);
    source.copy(dest, min((size_t)exactSize, source.length()));
}


void StorageManager::initializeFiles() {

    int initialHeader = -1;

    fstream docFile("doctors.dat", ios::in | ios::out | ios::binary);

    if (!docFile.is_open()) {
        ofstream outDoc("doctors.dat", ios::binary);

        if (outDoc.is_open()) {
            outDoc.write(
                reinterpret_cast<const char*>(&initialHeader),
                HEADER_SIZE
            );
        }

        outDoc.close();
    }
    else {
        docFile.seekg(0, ios::end);

        if (docFile.tellg() == 0) {
            docFile.clear();
            docFile.seekp(0, ios::beg);

            docFile.write(
                reinterpret_cast<const char*>(&initialHeader),
                HEADER_SIZE
            );
        }

        docFile.close();
    }


    fstream appointmentFile(
        "appointments.dat",
        ios::in | ios::out | ios::binary
    );

    if (!appointmentFile.is_open()) {
        ofstream outAppointment("appointments.dat", ios::binary);

        if (outAppointment.is_open()) {
            outAppointment.write(
                reinterpret_cast<const char*>(&initialHeader),
                HEADER_SIZE
            );
        }

        outAppointment.close();
    }
    else {
        appointmentFile.seekg(0, ios::end);

        if (appointmentFile.tellg() == 0) {
            appointmentFile.clear();
            appointmentFile.seekp(0, ios::beg);

            appointmentFile.write(
                reinterpret_cast<const char*>(&initialHeader),
                HEADER_SIZE
            );
        }

        appointmentFile.close();
    }
}


int StorageManager::readHeader(const string& filename) {

    ifstream file(filename, ios::binary);

    if (!file.is_open())
        return -1;

    int headerVal = -1;

    file.read(
        reinterpret_cast<char*>(&headerVal),
        HEADER_SIZE
    );

    if (file.gcount() != HEADER_SIZE) {
        file.close();
        return -1;
    }

    file.close();

    return headerVal;
}


bool StorageManager::writeHeader(
    const string& filename,
    int newHeaderVal
) {

    fstream file(filename, ios::in | ios::out | ios::binary);

    if (!file.is_open())
        return false;

    file.seekp(0, ios::beg);

    if (!file) {
        file.close();
        return false;
    }

    file.write(
        reinterpret_cast<const char*>(&newHeaderVal),
        HEADER_SIZE
    );

    bool success = file.good();

    file.close();

    return success;
}


bool StorageManager::writeAt(
    const string& filename,
    int offset,
    const char* buffer,
    int size
) {

    if (offset < 0 || size <= 0 || buffer == nullptr)
        return false;

    fstream file(filename, ios::in | ios::out | ios::binary);

    if (!file.is_open())
        return false;

    file.seekp(offset, ios::beg);

    if (!file) {
        file.close();
        return false;
    }

    file.write(buffer, size);

    bool success = file.good();

    file.close();

    return success;
}


bool StorageManager::readAt(
    const string& filename,
    int offset,
    char* buffer,
    int size
) {

    if (offset < 0 || size <= 0 || buffer == nullptr)
        return false;

    ifstream file(filename, ios::binary);

    if (!file.is_open())
        return false;

    file.seekg(offset, ios::beg);

    if (!file) {
        file.close();
        return false;
    }

    file.read(buffer, size);

    bool success = file.gcount() == size;

    file.close();

    return success;
}


void StorageManager::saveDoctor(
    const string& id,
    const string& name,
    const string& specialty,
    const string& address,
    char buffer[DOCTOR_RECORD_SIZE]
) {

    padString(id, buffer, 15);
    padString(name, buffer + 15, 30);
    padString(specialty, buffer + 45, 20);
    padString(address, buffer + 65, 30);
}


void StorageManager::saveAppointment(
    const string& appId,
    const string& appDate,
    const string& docId,
    char buffer[APPOINTMENT_RECORD_SIZE]
) {

    padString(appId, buffer, 15);
    padString(appDate, buffer + 15, 30);
    padString(docId, buffer + 45, 15);
}


bool StorageManager::writeDoctorRecord(
    int RRN,
    const char buffer[DOCTOR_RECORD_SIZE]
) {

    if (RRN < 0)
        return false;

    long long offset =
        HEADER_SIZE + (long long)RRN * DOCTOR_RECORD_SIZE;

    if (offset > INT_MAX)
        return false;

    return writeAt(
        "doctors.dat",
        (int)offset,
        buffer,
        DOCTOR_RECORD_SIZE
    );
}


bool StorageManager::writeAppointmentRecord(
    int RRN,
    const char buffer[APPOINTMENT_RECORD_SIZE]
) {

    if (RRN < 0)
        return false;

    long long offset =
        HEADER_SIZE + (long long)RRN * APPOINTMENT_RECORD_SIZE;

    if (offset > INT_MAX)
        return false;

    return writeAt(
        "appointments.dat",
        (int)offset,
        buffer,
        APPOINTMENT_RECORD_SIZE
    );
}

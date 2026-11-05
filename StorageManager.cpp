#include "StorageManager.h"
#include <fstream>
#include <cstring>
#include <algorithm>

using namespace std;

// Records padding
void StorageManager::padString(const string& source, char* dest, int exactSize) {
    memset(dest, ' ', exactSize);
    source.copy(dest, min((size_t)exactSize, source.length()));
}


// Initialize both files
void StorageManager::initializeFiles() {

    int initialHeader = -1;

    // Initialize Doctors File
    fstream docFile("doctors.dat", ios::in | ios::out | ios::binary);

    if (!docFile) {
        ofstream outDoc("doctors.dat", ios::binary);
        outDoc.write(reinterpret_cast<char*>(&initialHeader), HEADER_SIZE);
        outDoc.close();
    }
    else {
        docFile.seekg(0, ios::end);

        if (docFile.tellg() == 0) {
            docFile.seekp(0, ios::beg);
            docFile.write(reinterpret_cast<char*>(&initialHeader), HEADER_SIZE);
        }

        docFile.close();
    }


    // Initialize Appointments File
    fstream appointmentFile("appointments.dat", ios::in | ios::out | ios::binary);

    if (!appointmentFile) {
        ofstream outAppointment("appointments.dat", ios::binary);
        outAppointment.write(reinterpret_cast<char*>(&initialHeader), HEADER_SIZE);
        outAppointment.close();
    }
    else {
        appointmentFile.seekg(0, ios::end);

        if (appointmentFile.tellg() == 0) {
            appointmentFile.seekp(0, ios::beg);
            appointmentFile.write(reinterpret_cast<char*>(&initialHeader), HEADER_SIZE);
        }

        appointmentFile.close();
    }
}


// Read the Header Record
int StorageManager::readHeader(const string& filename) {

    ifstream file(filename, ios::binary);

    int headerVal = -1;

    if (file.is_open()) {
        file.read(reinterpret_cast<char*>(&headerVal), HEADER_SIZE);
        file.close();
    }

    return headerVal;
}


// Write / Update the Header Record
void StorageManager::writeHeader(const string& filename, int newHeaderVal) {

    fstream file(filename, ios::in | ios::out | ios::binary);

    if (file.is_open()) {
        file.seekp(0, ios::beg);
        file.write(reinterpret_cast<char*>(&newHeaderVal), HEADER_SIZE);
        file.close();
    }
}


// Write at a specific byte offset
void StorageManager::writeAt(const string& filename, int offset,const char* buffer, int size) {

    if (offset < 0 || size < 0)
        return;

    fstream file(filename, ios::in | ios::out | ios::binary);

    if (file.is_open()) {
        file.seekp(offset, ios::beg);
        file.write(buffer, size);
        file.close();
    }
}


// Read from a specific byte offset
void StorageManager::readAt(const string& filename, int offset,char* buffer, int size) {

    if (offset < 0 || size < 0)
        return;

    ifstream file(filename, ios::binary);

    if (file.is_open()) {
        file.seekg(offset, ios::beg);
        file.read(buffer, size);
        file.close();
    }
}


// Format a Doctor record 
void StorageManager::saveDoctor(const string& id,const string& name, const string& specialty,const string& address,char buffer[DOCTOR_RECORD_SIZE]) {

    padString(id, buffer, 15);
    padString(name, buffer + 15, 30);
    padString(specialty, buffer + 45, 20);
    padString(address, buffer + 65, 30);
}


// Format an Appointment record
void StorageManager::saveAppointment(const string& appId,const string& appDate,const string& docId,char buffer[APPOINTMENT_RECORD_SIZE]) {

    padString(appId, buffer, 15);
    padString(appDate, buffer + 15, 30);
    padString(docId, buffer + 45, 15);
}


// Write a Doctor record using its RRN
void StorageManager::writeDoctorRecord(int RRN,const char buffer[DOCTOR_RECORD_SIZE]) {

    if (RRN < 0)
        return;

    int offset = HEADER_SIZE + (RRN * DOCTOR_RECORD_SIZE);

    writeAt("doctors.dat", offset, buffer, DOCTOR_RECORD_SIZE);
}


// Write an Appointment record using its RRN
void StorageManager::writeAppointmentRecord(int RRN,const char buffer[APPOINTMENT_RECORD_SIZE]) {

    int offset = HEADER_SIZE + (RRN * APPOINTMENT_RECORD_SIZE);

    writeAt("appointments.dat", offset, buffer, APPOINTMENT_RECORD_SIZE);
}

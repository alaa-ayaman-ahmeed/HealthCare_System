
#include "OperationsManager.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <climits>

using namespace std;


// Validate a Free List record offset.
static bool isValidFreeOffset(
    StorageManager& storage,
    const string& filename,
    int offset,
    int recordSize,
    long long fileEnd
) {
    if (offset < HEADER_SIZE)
        return false;

    if ((offset - HEADER_SIZE) % recordSize != 0)
        return false;

    if (fileEnd < HEADER_SIZE + recordSize)
        return false;

    if (offset > fileEnd - recordSize)
        return false;

    char marker;

    if (!storage.readAt(filename, offset, &marker, 1))
        return false;

    return marker == '*';
}


// Validate the Free List head and its next pointer.
static bool isValidFreeHead(
    StorageManager& storage,
    const string& filename,
    int head,
    int recordSize,
    long long fileEnd
) {
    if (head == -1)
        return true;

    if (!isValidFreeOffset(
            storage, filename, head, recordSize, fileEnd))
        return false;

    int nextOffset;

    if (!storage.readAt(
            filename,
            head + 1,
            reinterpret_cast<char*>(&nextOffset),
            sizeof(int)))
        return false;

    if (nextOffset == -1)
        return true;

    return isValidFreeOffset(
        storage, filename, nextOffset, recordSize, fileEnd);
}


OperationsManager::OperationsManager(
    StorageManager& sm,
    PrimaryIndex& pi,
    SecondaryIndex& nameIdx,
    SecondaryIndex& specIdx,
    SecondaryIndex& appIdx
)
    : storage(sm),
      primary(pi),
      docNameIdx(nameIdx),
      docSpecIdx(specIdx),
      appDocIdx(appIdx) {
}


long long OperationsManager::getFileEnd(const string& filename) {

    ifstream file(filename, ios::binary | ios::ate);

    if (!file.is_open())
        return -1;

    long long result = static_cast<long long>(file.tellg());

    file.close();

    return result;
}


string OperationsManager::cleanField(const char* data, int size) {

    string result(data, size);

    while (!result.empty() &&
           (result.back() == ' ' || result.back() == '\0')) {
        result.pop_back();
    }

    return result;
}


int OperationsManager::getDoctorOffset(const string& id) {

    int pos = primary.findDoctor(id);

    if (pos == -1)
        return -1;

    return primary.getDoctorsIndex()[pos].offset;
}


int OperationsManager::getAppointmentOffset(const string& id) {

    int pos = primary.findAppointment(id);

    if (pos == -1)
        return -1;

    return primary.getAppointmentsIndex()[pos].offset;
}


bool OperationsManager::validDoctor(
    const string& id,
    const string& name,
    const string& specialty,
    const string& address
) {

    return !id.empty() &&
           !name.empty() &&
           !specialty.empty() &&
           !address.empty() &&
           id.size() <= 15 &&
           name.size() <= 30 &&
           specialty.size() <= 20 &&
           address.size() <= 30;
}


bool OperationsManager::validAppointment(
    const string& id,
    const string& date,
    const string& doctorID
) {

    return !id.empty() &&
           !date.empty() &&
           !doctorID.empty() &&
           id.size() <= 15 &&
           date.size() <= 30 &&
           doctorID.size() <= 15;
}


// ==========================================
// ADD DOCTOR
// ==========================================

bool OperationsManager::addDoctor(
    const string& id,
    const string& name,
    const string& specialty,
    const string& address
) {

    if (!validDoctor(id, name, specialty, address)) {
        cout << "Invalid Doctor data or field size exceeded!\n";
        return false;
    }

    if (primary.findDoctor(id) != -1) {
        cout << "Doctor already exists!\n";
        return false;
    }

    const string filename = "doctors.dat";

    long long fileEnd = getFileEnd(filename);

    if (fileEnd < HEADER_SIZE ||
        fileEnd > INT_MAX ||
        (fileEnd - HEADER_SIZE) % DOCTOR_RECORD_SIZE != 0) {
        cout << "Error: Doctors file has an invalid size!\n";
        return false;
    }

    int oldHead = storage.readHeader(filename);

    if (!isValidFreeHead(
            storage, filename, oldHead,
            DOCTOR_RECORD_SIZE, fileEnd)) {
        cout << "Error: Invalid Doctors Free List!\n";
        return false;
    }

    int offset;
    int nextOffset = -1;
    char oldRecord[DOCTOR_RECORD_SIZE];

    bool reusing = (oldHead != -1);

    if (reusing) {

        offset = oldHead;

        if (!storage.readAt(
                filename, offset,
                oldRecord, DOCTOR_RECORD_SIZE)) {
            cout << "Error: Could not read deleted Doctor!\n";
            return false;
        }

        if (!storage.readAt(
                filename, offset + 1,
                reinterpret_cast<char*>(&nextOffset),
                sizeof(int))) {
            cout << "Error: Could not read Free List pointer!\n";
            return false;
        }
    }
    else {
        offset = static_cast<int>(fileEnd);
    }

    char buffer[DOCTOR_RECORD_SIZE];
    storage.saveDoctor(id, name, specialty, address, buffer);

    if (!storage.writeAt(
            filename, offset, buffer, DOCTOR_RECORD_SIZE)) {
        cout << "Error: Could not write Doctor record!\n";
        return false;
    }

    if (reusing && !storage.writeHeader(filename, nextOffset)) {

        storage.writeAt(
            filename, offset, oldRecord, DOCTOR_RECORD_SIZE);

        cout << "Error: Could not update Doctors Free List!\n";
        return false;
    }

    if (!primary.addDoctor(id, offset)) {
        cout << "Could not update Doctor primary index!\n";
        return false;
    }

    if (!docNameIdx.add(name, id))
        cout << "Warning: Could not update Doctor Name index!\n";

    if (!docSpecIdx.add(specialty, id))
        cout << "Warning: Could not update Doctor Specialty index!\n";

    cout << "Doctor added successfully!\n";
    return true;
}


// ==========================================
// ADD APPOINTMENT
// ==========================================

bool OperationsManager::addAppointment(
    const string& id,
    const string& date,
    const string& doctorID
) {

    if (!validAppointment(id, date, doctorID)) {
        cout << "Invalid Appointment data or field size exceeded!\n";
        return false;
    }

    if (primary.findAppointment(id) != -1) {
        cout << "Appointment already exists!\n";
        return false;
    }

    if (primary.findDoctor(doctorID) == -1) {
        cout << "Doctor does not exist!\n";
        return false;
    }

    const string filename = "appointments.dat";

    long long fileEnd = getFileEnd(filename);

    if (fileEnd < HEADER_SIZE ||
        fileEnd > INT_MAX ||
        (fileEnd - HEADER_SIZE) % APPOINTMENT_RECORD_SIZE != 0) {
        cout << "Error: Appointments file has an invalid size!\n";
        return false;
    }

    int oldHead = storage.readHeader(filename);

    if (!isValidFreeHead(
            storage, filename, oldHead,
            APPOINTMENT_RECORD_SIZE, fileEnd)) {
        cout << "Error: Invalid Appointments Free List!\n";
        return false;
    }

    int offset;
    int nextOffset = -1;
    char oldRecord[APPOINTMENT_RECORD_SIZE];

    bool reusing = (oldHead != -1);

    if (reusing) {

        offset = oldHead;

        if (!storage.readAt(
                filename, offset,
                oldRecord, APPOINTMENT_RECORD_SIZE)) {
            cout << "Error: Could not read deleted Appointment!\n";
            return false;
        }

        if (!storage.readAt(
                filename, offset + 1,
                reinterpret_cast<char*>(&nextOffset),
                sizeof(int))) {
            cout << "Error: Could not read Free List pointer!\n";
            return false;
        }
    }
    else {
        offset = static_cast<int>(fileEnd);
    }

    char buffer[APPOINTMENT_RECORD_SIZE];
    storage.saveAppointment(id, date, doctorID, buffer);

    if (!storage.writeAt(
            filename, offset, buffer, APPOINTMENT_RECORD_SIZE)) {
        cout << "Error: Could not write Appointment record!\n";
        return false;
    }

    if (reusing && !storage.writeHeader(filename, nextOffset)) {

        storage.writeAt(
            filename, offset, oldRecord, APPOINTMENT_RECORD_SIZE);

        cout << "Error: Could not update Appointments Free List!\n";
        return false;
    }

    if (!primary.addAppointment(id, offset)) {
        cout << "Could not update Appointment primary index!\n";
        return false;
    }

    if (!appDocIdx.add(doctorID, id))
        cout << "Warning: Could not update Appointment secondary index!\n";

    cout << "Appointment added successfully!\n";
    return true;
}


// ==========================================
// DELETE DOCTOR
// ==========================================

bool OperationsManager::deleteDoctor(const string& id) {

    int offset = getDoctorOffset(id);

    if (offset == -1) {
        cout << "Doctor does not exist!\n";
        return false;
    }

    vector<string> appointments = appDocIdx.search(id);

    if (!appointments.empty()) {
        cout << "Cannot delete Doctor: appointments still exist!\n";
        return false;
    }

    const string filename = "doctors.dat";

    long long fileEnd = getFileEnd(filename);

    if (offset < HEADER_SIZE ||
        (offset - HEADER_SIZE) % DOCTOR_RECORD_SIZE != 0 ||
        fileEnd < HEADER_SIZE + DOCTOR_RECORD_SIZE ||
        offset > fileEnd - DOCTOR_RECORD_SIZE) {
        cout << "Error: Invalid Doctor record offset!\n";
        return false;
    }

    char originalRecord[DOCTOR_RECORD_SIZE];

    if (!storage.readAt(
            filename, offset,
            originalRecord, DOCTOR_RECORD_SIZE)) {
        cout << "Error: Could not read Doctor record!\n";
        return false;
    }

    if (originalRecord[0] == '*') {
        cout << "Error: Doctor is already deleted!\n";
        return false;
    }

    string name = cleanField(originalRecord + 15, 30);
    string specialty = cleanField(originalRecord + 45, 20);

    int oldHead = storage.readHeader(filename);

    if (!isValidFreeHead(
            storage, filename, oldHead,
            DOCTOR_RECORD_SIZE, fileEnd)) {
        cout << "Error: Invalid Doctors Free List!\n";
        return false;
    }

    char deletedRecord[DOCTOR_RECORD_SIZE];

    memcpy(deletedRecord, originalRecord, DOCTOR_RECORD_SIZE);

    deletedRecord[0] = '*';

    memcpy(
        deletedRecord + 1,
        &oldHead,
        sizeof(int)
    );

    if (!storage.writeAt(
            filename, offset,
            deletedRecord, DOCTOR_RECORD_SIZE)) {
        cout << "Error: Could not mark Doctor as deleted!\n";
        return false;
    }

    if (!storage.writeHeader(filename, offset)) {

        storage.writeAt(
            filename, offset,
            originalRecord, DOCTOR_RECORD_SIZE);

        cout << "Error: Could not update Doctors Free List!\n";
        return false;
    }

    primary.deleteDoctor(id);

    docNameIdx.remove(name, id);
    docSpecIdx.remove(specialty, id);

    cout << "Doctor deleted successfully!\n";
    return true;
}


// ==========================================
// DELETE APPOINTMENT
// ==========================================

bool OperationsManager::deleteAppointment(const string& id) {

    int offset = getAppointmentOffset(id);

    if (offset == -1) {
        cout << "Appointment does not exist!\n";
        return false;
    }

    const string filename = "appointments.dat";

    long long fileEnd = getFileEnd(filename);

    if (offset < HEADER_SIZE ||
        (offset - HEADER_SIZE) % APPOINTMENT_RECORD_SIZE != 0 ||
        fileEnd < HEADER_SIZE + APPOINTMENT_RECORD_SIZE ||
        offset > fileEnd - APPOINTMENT_RECORD_SIZE) {
        cout << "Error: Invalid Appointment record offset!\n";
        return false;
    }

    char originalRecord[APPOINTMENT_RECORD_SIZE];

    if (!storage.readAt(
            filename, offset,
            originalRecord, APPOINTMENT_RECORD_SIZE)) {
        cout << "Error: Could not read Appointment record!\n";
        return false;
    }

    if (originalRecord[0] == '*') {
        cout << "Error: Appointment is already deleted!\n";
        return false;
    }

    string doctorID = cleanField(originalRecord + 45, 15);

    int oldHead = storage.readHeader(filename);

    if (!isValidFreeHead(
            storage, filename, oldHead,
            APPOINTMENT_RECORD_SIZE, fileEnd)) {
        cout << "Error: Invalid Appointments Free List!\n";
        return false;
    }

    char deletedRecord[APPOINTMENT_RECORD_SIZE];

    memcpy(deletedRecord, originalRecord, APPOINTMENT_RECORD_SIZE);

    deletedRecord[0] = '*';

    memcpy(
        deletedRecord + 1,
        &oldHead,
        sizeof(int)
    );

    if (!storage.writeAt(
            filename, offset,
            deletedRecord, APPOINTMENT_RECORD_SIZE)) {
        cout << "Error: Could not mark Appointment as deleted!\n";
        return false;
    }

    if (!storage.writeHeader(filename, offset)) {

        storage.writeAt(
            filename, offset,
            originalRecord, APPOINTMENT_RECORD_SIZE);

        cout << "Error: Could not update Appointments Free List!\n";
        return false;
    }

    primary.deleteAppointment(id);

    appDocIdx.remove(doctorID, id);

    cout << "Appointment deleted successfully!\n";
    return true;
}


// ==========================================
// UPDATE DOCTOR NAME
// ==========================================

bool OperationsManager::updateDoctorName(
    const string& id,
    const string& newName
) {

    if (newName.empty() || newName.size() > 30) {
        cout << "Invalid Doctor Name!\n";
        return false;
    }

    int offset = getDoctorOffset(id);

    if (offset == -1) {
        cout << "Doctor does not exist!\n";
        return false;
    }

    char oldBuffer[30];

    if (!storage.readAt(
            "doctors.dat", offset + 15,
            oldBuffer, 30)) {
        cout << "Error: Could not read Doctor name!\n";
        return false;
    }

    string oldName = cleanField(oldBuffer, 30);

    char newBuffer[30];

    memset(newBuffer, ' ', 30);
    memcpy(newBuffer, newName.c_str(), newName.size());

    if (!storage.writeAt(
            "doctors.dat", offset + 15,
            newBuffer, 30)) {
        cout << "Error: Could not update Doctor name!\n";
        return false;
    }

    if (!docNameIdx.changeKey(oldName, newName, id)) {
        cout << "Warning: Could not update name index!\n";
        return false;
    }

    cout << "Doctor Name updated successfully!\n";
    return true;
}


// ==========================================
// UPDATE DOCTOR ID
// ==========================================

bool OperationsManager::updateDoctorID(
    const string& oldID,
    const string& newID
) {

    if (newID.empty() || newID.size() > 15) {
        cout << "Invalid Doctor ID!\n";
        return false;
    }

    int offset = getDoctorOffset(oldID);

    if (offset == -1) {
        cout << "Doctor does not exist!\n";
        return false;
    }

    if (oldID != newID &&
        primary.findDoctor(newID) != -1) {
        cout << "New Doctor ID already exists!\n";
        return false;
    }

    if (oldID == newID)
        return true;

    char record[DOCTOR_RECORD_SIZE];

    if (!storage.readAt(
            "doctors.dat", offset,
            record, DOCTOR_RECORD_SIZE)) {
        cout << "Error: Could not read Doctor record!\n";
        return false;
    }

    string name = cleanField(record + 15, 30);
    string specialty = cleanField(record + 45, 20);

    char newBuffer[15];

    memset(newBuffer, ' ', 15);
    memcpy(newBuffer, newID.c_str(), newID.size());

    if (!storage.writeAt(
            "doctors.dat", offset,
            newBuffer, 15)) {
        cout << "Error: Could not update Doctor ID!\n";
        return false;
    }

    if (!primary.updateDoctorID(oldID, newID)) {
        cout << "Could not update primary index!\n";
        return false;
    }

    docNameIdx.replacePrimary(name, oldID, newID);
    docSpecIdx.replacePrimary(specialty, oldID, newID);

    vector<string> appointments = appDocIdx.search(oldID);

    for (const string& appointmentID : appointments) {

        int appOffset = getAppointmentOffset(appointmentID);

        if (appOffset == -1)
            continue;

        char appDoctorID[15];

        memset(appDoctorID, ' ', 15);
        memcpy(appDoctorID, newID.c_str(), newID.size());

        if (!storage.writeAt(
                "appointments.dat",
                appOffset + 45,
                appDoctorID,
                15)) {
            cout << "Warning: Could not update an Appointment record!\n";
        }
    }

    appDocIdx.renameKey(oldID, newID);

    cout << "Doctor ID updated successfully!\n";
    return true;
}


// ==========================================
// UPDATE APPOINTMENT DATE
// ==========================================

bool OperationsManager::updateAppointmentDate(
    const string& id,
    const string& newDate
) {

    if (newDate.empty() || newDate.size() > 30) {
        cout << "Invalid Appointment Date!\n";
        return false;
    }

    int offset = getAppointmentOffset(id);

    if (offset == -1) {
        cout << "Appointment does not exist!\n";
        return false;
    }

    char newBuffer[30];

    memset(newBuffer, ' ', 30);
    memcpy(newBuffer, newDate.c_str(), newDate.size());

    if (!storage.writeAt(
            "appointments.dat",
            offset + 15,
            newBuffer,
            30)) {
        cout << "Error: Could not update Appointment date!\n";
        return false;
    }

    cout << "Appointment Date updated successfully!\n";
    return true;
}

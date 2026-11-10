
#include "OperationsManager.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>

using namespace std;




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




// Get the end of a data file.
// Used when there is no deleted space to reuse.

int OperationsManager::getFileEnd(const string& filename) {

    ifstream file(filename, ios::binary | ios::ate);

    if (!file.is_open())
        return -1;

    return (int)file.tellg();
}


// Remove trailing spaces from a field.

string OperationsManager::cleanField(const char* data, int size) {

    string result(data, size);

    while (!result.empty() &&
           (result.back() == ' ' || result.back() == '\0')) {

        result.pop_back();
    }

    return result;
}


// Get the actual file offset of a Doctor.

int OperationsManager::getDoctorOffset(const string& id) {

    int pos = primary.findDoctor(id);

    if (pos == -1)
        return -1;

    return primary.getDoctorsIndex()[pos].offset;
}


// Get the actual file offset of an Appointment.

int OperationsManager::getAppointmentOffset(const string& id) {

    int pos = primary.findAppointment(id);

    if (pos == -1)
        return -1;

    return primary.getAppointmentsIndex()[pos].offset;
}


// Check Doctor fields.

bool OperationsManager::validDoctor(
    const string& id,
    const string& name,
    const string& specialty,
    const string& address) {

    return !id.empty() &&
           !name.empty() &&
           !specialty.empty() &&
           !address.empty() &&
           id.size() <= 15 &&
           name.size() <= 30 &&
           specialty.size() <= 20 &&
           address.size() <= 30;
}


// Check Appointment fields.

bool OperationsManager::validAppointment(
    const string& id,
    const string& date,
    const string& doctorID) {

    return !id.empty() &&
           !date.empty() &&
           !doctorID.empty() &&
           id.size() <= 15 &&
           date.size() <= 30 &&
           doctorID.size() <= 15;
}



// ADD DOCTOR


bool OperationsManager::addDoctor(
    const string& id,
    const string& name,
    const string& specialty,
    const string& address) {

    // 1. Validate input.

    if (!validDoctor(id, name, specialty, address)) {

        cout << "Invalid Doctor data or field size exceeded!\n";
        return false;
    }


    // 2. Check duplicate Doctor ID.

    if (primary.findDoctor(id) != -1) {

        cout << "Doctor already exists!\n";
        return false;
    }


    // 3. Prepare the Doctor record.

    char buffer[DOCTOR_RECORD_SIZE];

    storage.saveDoctor(
        id,
        name,
        specialty,
        address,
        buffer
    );


    // 4. Check the AVAIL LIST.

    int offset = storage.readHeader("doctors.dat");

    if (offset != -1) {

        // There is a deleted record available.

        int nextOffset;

        storage.readAt(
            "doctors.dat",
            offset + 1,
            reinterpret_cast<char*>(&nextOffset),
            sizeof(int)
        );

        // Remove the first deleted record from AVAIL LIST.

        storage.writeHeader("doctors.dat", nextOffset);

    }
    else {

        // No deleted records.
        // Append at the end of the file.

        offset = getFileEnd("doctors.dat");

        if (offset < 0) {
            cout << "Cannot open Doctors file!\n";
            return false;
        }
    }


    // 5. Write the new Doctor.

    storage.writeAt(
        "doctors.dat",
        offset,
        buffer,
        DOCTOR_RECORD_SIZE
    );


    // 6. Update Primary Index.

    if (!primary.addDoctor(id, offset)) {

        cout << "Could not update Doctor primary index!\n";
        return false;
    }


    // 7. Update Secondary Indexes.

    docNameIdx.add(name, id);

    docSpecIdx.add(specialty, id);


    cout << "Doctor added successfully!\n";

    return true;
}


// ==========================================
// ADD APPOINTMENT
// ==========================================

bool OperationsManager::addAppointment(
    const string& id,
    const string& date,
    const string& doctorID) {

    // 1. Validate input.

    if (!validAppointment(id, date, doctorID)) {

        cout << "Invalid Appointment data or field size exceeded!\n";
        return false;
    }


    // 2. Check duplicate Appointment ID.

    if (primary.findAppointment(id) != -1) {

        cout << "Appointment already exists!\n";
        return false;
    }


    // 3. Check that the Doctor exists.

    if (primary.findDoctor(doctorID) == -1) {

        cout << "Doctor does not exist!\n";
        return false;
    }


    // 4. Prepare Appointment record.

    char buffer[APPOINTMENT_RECORD_SIZE];

    storage.saveAppointment(
        id,
        date,
        doctorID,
        buffer
    );


    // 5. Check AVAIL LIST.

    int offset = storage.readHeader("appointments.dat");

    if (offset != -1) {

        int nextOffset;

        storage.readAt(
            "appointments.dat",
            offset + 1,
            reinterpret_cast<char*>(&nextOffset),
            sizeof(int)
        );

        storage.writeHeader("appointments.dat", nextOffset);

    }
    else {

        offset = getFileEnd("appointments.dat");

        if (offset < 0) {
            cout << "Cannot open Appointments file!\n";
            return false;
        }
    }


    // 6. Write Appointment.

    storage.writeAt(
        "appointments.dat",
        offset,
        buffer,
        APPOINTMENT_RECORD_SIZE
    );


    // 7. Update Primary Index.

    if (!primary.addAppointment(id, offset)) {

        cout << "Could not update Appointment primary index!\n";
        return false;
    }


    // 8. Update Secondary Index.

    appDocIdx.add(doctorID, id);


    cout << "Appointment added successfully!\n";

    return true;
}


// ==========================================
// DELETE DOCTOR
// ==========================================

bool OperationsManager::deleteDoctor(const string& id) {

    // 1. Find Doctor.

    int offset = getDoctorOffset(id);

    if (offset == -1) {

        cout << "Doctor does not exist!\n";
        return false;
    }


    // 2. Do not delete a Doctor who still has appointments.

    vector<string> appointments = appDocIdx.search(id);

    if (!appointments.empty()) {

        cout << "Cannot delete Doctor: appointments still exist!\n";
        return false;
    }


    // 3. Read the Doctor record.

    char buffer[DOCTOR_RECORD_SIZE];

    storage.readAt(
        "doctors.dat",
        offset,
        buffer,
        DOCTOR_RECORD_SIZE
    );

    string name = cleanField(buffer + 15, 30);
    string specialty = cleanField(buffer + 45, 20);


    // 4. Get the current AVAIL LIST head.

    int oldHead = storage.readHeader("doctors.dat");


    // 5. Mark the record as deleted.

    char marker = '*';

    storage.writeAt(
        "doctors.dat",
        offset,
        &marker,
        1
    );


    // 6. Store the old head inside this deleted record.

    storage.writeAt(
        "doctors.dat",
        offset + 1,
        reinterpret_cast<const char*>(&oldHead),
        sizeof(int)
    );


    // 7. Make this record the new AVAIL LIST head.

    storage.writeHeader("doctors.dat", offset);


    // 8. Remove from Primary Index.

    primary.deleteDoctor(id);


    // 9. Remove from Secondary Indexes.

    docNameIdx.remove(name, id);

    docSpecIdx.remove(specialty, id);


    cout << "Doctor deleted successfully!\n";

    return true;
}


// ==========================================
// DELETE APPOINTMENT
// ==========================================

bool OperationsManager::deleteAppointment(const string& id) {

    // 1. Find Appointment.

    int offset = getAppointmentOffset(id);

    if (offset == -1) {

        cout << "Appointment does not exist!\n";
        return false;
    }


    // 2. Read Appointment record.

    char buffer[APPOINTMENT_RECORD_SIZE];

    storage.readAt(
        "appointments.dat",
        offset,
        buffer,
        APPOINTMENT_RECORD_SIZE
    );

    string doctorID = cleanField(buffer + 45, 15);


    // 3. Get old AVAIL LIST head.

    int oldHead = storage.readHeader("appointments.dat");


    // 4. Mark record as deleted.

    char marker = '*';

    storage.writeAt(
        "appointments.dat",
        offset,
        &marker,
        1
    );


    // 5. Store old head as next pointer.

    storage.writeAt(
        "appointments.dat",
        offset + 1,
        reinterpret_cast<const char*>(&oldHead),
        sizeof(int)
    );


    // 6. Update Header.

    storage.writeHeader("appointments.dat", offset);


    // 7. Remove from Primary Index.

    primary.deleteAppointment(id);


    // 8. Remove from Secondary Index.

    appDocIdx.remove(doctorID, id);


    cout << "Appointment deleted successfully!\n";

    return true;
}


// ==========================================
// UPDATE DOCTOR NAME
// ==========================================

bool OperationsManager::updateDoctorName(
    const string& id,
    const string& newName) {

    // 1. Validate new name.

    if (newName.empty() || newName.size() > 30) {

        cout << "Invalid Doctor Name!\n";
        return false;
    }


    // 2. Find Doctor.

    int offset = getDoctorOffset(id);

    if (offset == -1) {

        cout << "Doctor does not exist!\n";
        return false;
    }


    // 3. Read old Doctor name.

    char buffer[30];

    storage.readAt(
        "doctors.dat",
        offset + 15,
        buffer,
        30
    );

    string oldName = cleanField(buffer, 30);


    // 4. Prepare new padded name.

    char newBuffer[30];

    memset(newBuffer, ' ', 30);

    memcpy(newBuffer, newName.c_str(), newName.size());


    // 5. Update Doctor Name in file.

    storage.writeAt(
        "doctors.dat",
        offset + 15,
        newBuffer,
        30
    );


    // 6. Update Secondary Index.

    if (!docNameIdx.changeKey(oldName, newName, id)) {

        cout << "Warning: Could not update name index!\n";
        return false;
    }


    cout << "Doctor Name updated successfully!\n";

    return true;
}


// ==========================================
// UPDATE DOCTOR ID (PRIMARY KEY)
// ==========================================

bool OperationsManager::updateDoctorID(
    const string& oldID,
    const string& newID) {

    // 1. Validate new ID.

    if (newID.empty() || newID.size() > 15) {

        cout << "Invalid Doctor ID!\n";
        return false;
    }


    // 2. Check old Doctor exists.

    int offset = getDoctorOffset(oldID);

    if (offset == -1) {

        cout << "Doctor does not exist!\n";
        return false;
    }


    // 3. Check duplicate new ID.

    if (oldID != newID &&
        primary.findDoctor(newID) != -1) {

        cout << "New Doctor ID already exists!\n";
        return false;
    }


    if (oldID == newID)
        return true;


    // 4. Read old Doctor record.

    char buffer[DOCTOR_RECORD_SIZE];

    storage.readAt(
        "doctors.dat",
        offset,
        buffer,
        DOCTOR_RECORD_SIZE
    );

    string name = cleanField(buffer + 15, 30);
    string specialty = cleanField(buffer + 45, 20);


    // 5. Prepare new ID.

    char newBuffer[15];

    memset(newBuffer, ' ', 15);

    memcpy(newBuffer, newID.c_str(), newID.size());


    // 6. Update Doctor ID in data file.

    storage.writeAt(
        "doctors.dat",
        offset,
        newBuffer,
        15
    );


    // 7. Update Primary Index (sorted insertion).

    if (!primary.updateDoctorID(oldID, newID)) {

        cout << "Could not update primary index!\n";
        return false;
    }


    // 8. Update Doctor Name secondary index.

    docNameIdx.replacePrimary(
        name,
        oldID,
        newID
    );


    // 9. Update Doctor Specialty secondary index.

    docSpecIdx.replacePrimary(
        specialty,
        oldID,
        newID
    );


    // 10. Update all Appointment records that reference old ID.

    vector<string> appointments = appDocIdx.search(oldID);

    for (const string& appointmentID : appointments) {

        int appOffset = getAppointmentOffset(appointmentID);

        if (appOffset == -1)
            continue;

        char appDoctorID[15];

        memset(appDoctorID, ' ', 15);

        memcpy(
            appDoctorID,
            newID.c_str(),
            newID.size()
        );

        storage.writeAt(
            "appointments.dat",
            appOffset + 45,
            appDoctorID,
            15
        );
    }


    // 11. Update Appointment Doctor ID secondary index.

    appDocIdx.renameKey(oldID, newID);


    cout << "Doctor ID updated successfully!\n";

    return true;
}


// ==========================================
// UPDATE APPOINTMENT DATE
// ==========================================

bool OperationsManager::updateAppointmentDate(
    const string& id,
    const string& newDate) {

    // 1. Validate date.

    if (newDate.empty() || newDate.size() > 30) {

        cout << "Invalid Appointment Date!\n";
        return false;
    }


    // 2. Find Appointment.

    int offset = getAppointmentOffset(id);

    if (offset == -1) {

        cout << "Appointment does not exist!\n";
        return false;
    }


    // 3. Prepare new date.

    char newBuffer[30];

    memset(newBuffer, ' ', 30);

    memcpy(newBuffer, newDate.c_str(), newDate.size());


    // 4. Update date directly in file.

    storage.writeAt(
        "appointments.dat",
        offset + 15,
        newBuffer,
        30
    );


    cout << "Appointment Date updated successfully!\n";

    return true;
}

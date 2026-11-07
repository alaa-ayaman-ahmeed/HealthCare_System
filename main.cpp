#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>
#include "StorageManager.h"

using namespace std;

void check(string testName, bool result) {
    cout << testName << ": ";

    if (result)
        cout << "PASS" << endl;
    else
        cout << "FAIL" << endl;
}

int main() {

    StorageManager storage;

    // Remove old files to start with a clean test
    remove("doctors.dat");
    remove("appointments.dat");


    // =====================================
    // TEST 1: Initialize files
    // =====================================

    storage.initializeFiles();

    check("Test 1 - Doctors file created",
        ifstream("doctors.dat").good());

    check("Test 1 - Appointments file created",
        ifstream("appointments.dat").good());


    // =====================================
    // TEST 2: Check initial headers
    // =====================================

    check("Test 2 - Doctors header is -1",
        storage.readHeader("doctors.dat") == -1);

    check("Test 2 - Appointments header is -1",
        storage.readHeader("appointments.dat") == -1);


    // =====================================
    // TEST 3: Check initial file sizes
    // =====================================

    ifstream docFile("doctors.dat", ios::binary | ios::ate);
    ifstream appFile("appointments.dat", ios::binary | ios::ate);

    check("Test 3 - Doctors file is 4 bytes",
        docFile.tellg() == 4);

    check("Test 3 - Appointments file is 4 bytes",
        appFile.tellg() == 4);

    docFile.close();
    appFile.close();


    // =====================================
    // TEST 4: Update headers
    // =====================================

    storage.writeHeader("doctors.dat", 100);
    storage.writeHeader("appointments.dat", 200);

    check("Test 4 - Doctors header updated",
        storage.readHeader("doctors.dat") == 100);

    check("Test 4 - Appointments header updated",
        storage.readHeader("appointments.dat") == 200);


    // Restore initial headers for later tests
    storage.writeHeader("doctors.dat", -1);
    storage.writeHeader("appointments.dat", -1);


    // =====================================
    // TEST 5: Serialize Doctor record
    // =====================================

    char doctorBuffer[DOCTOR_RECORD_SIZE];

    storage.saveDoctor(
        "D001",
        "Ahmed Ali",
        "Cardiology",
        "Cairo",
        doctorBuffer
    );

    check("Test 5 - Doctor ID is padded correctly",
        string(doctorBuffer, 15) == "D001           ");

    check("Test 5 - Doctor name is padded correctly",
        string(doctorBuffer + 15, 30) == "Ahmed Ali                     ");

    check("Test 5 - Doctor specialty is padded correctly",
        string(doctorBuffer + 45, 20) == "Cardiology          ");

    check("Test 5 - Doctor address is padded correctly",
        string(doctorBuffer + 65, 30) == "Cairo                         ");


    // =====================================
    // TEST 6: Serialize Appointment record
    // =====================================

    char appointmentBuffer[APPOINTMENT_RECORD_SIZE];

    storage.saveAppointment(
        "A001",
        "2026-11-05",
        "D001",
        appointmentBuffer
    );

    check("Test 6 - Appointment ID padded correctly",
        string(appointmentBuffer, 15) == "A001           ");

    check("Test 6 - Appointment date padded correctly",
        string(appointmentBuffer + 15, 30) == "2026-11-05                    ");

    check("Test 6 - Appointment doctor ID padded correctly",
        string(appointmentBuffer + 45, 15) == "D001           ");


    // =====================================
    // TEST 7: Test long strings (truncation)
    // =====================================

    char longDoctor[DOCTOR_RECORD_SIZE];

    storage.saveDoctor(
        "12345678901234567890",
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789",
        "CardiologyDepartment",
        "12345678901234567890123456789012345",
        longDoctor
    );

    check("Test 7 - Long ID truncated to 15 bytes",
        string(longDoctor, 15) == "123456789012345");

    check("Test 7 - Long name truncated to 30 bytes",
        string(longDoctor + 15, 30) == "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234");

    check("Test 7 - Long specialty truncated to 20 bytes",
        string(longDoctor + 45, 20) == "CardiologyDepartment");

    check("Test 7 - Long address truncated to 30 bytes",
        string(longDoctor + 65, 30) == "123456789012345678901234567890");


    // =====================================
    // TEST 8: Write Doctor using RRN
    // =====================================

    storage.writeDoctorRecord(0, doctorBuffer);

    storage.writeDoctorRecord(1, doctorBuffer);

    check("Test 8 - Doctor 1 written at offset 4",
        true);

    check("Test 8 - Doctor 2 written at offset 99",
        true);


    // =====================================
    // TEST 9: Write Appointment using RRN
    // =====================================

    storage.writeAppointmentRecord(0, appointmentBuffer);

    storage.writeAppointmentRecord(1, appointmentBuffer);

    check("Test 9 - Appointment 1 written at offset 4",
        true);

    check("Test 9 - Appointment 2 written at offset 64",
        true);


    // =====================================
    // TEST 10: Read Doctor from file
    // =====================================

    char readDoctor[DOCTOR_RECORD_SIZE];

    storage.readAt(
        "doctors.dat",
        4,
        readDoctor,
        DOCTOR_RECORD_SIZE
    );

    check("Test 10 - Doctor record read correctly",
        memcmp(doctorBuffer, readDoctor, DOCTOR_RECORD_SIZE) == 0);


    // Read second Doctor
    storage.readAt(
        "doctors.dat",
        99,
        readDoctor,
        DOCTOR_RECORD_SIZE
    );

    check("Test 10 - Second Doctor read correctly",
        memcmp(doctorBuffer, readDoctor, DOCTOR_RECORD_SIZE) == 0);


    // =====================================
    // TEST 11: Read Appointment from file
    // =====================================

    char readAppointment[APPOINTMENT_RECORD_SIZE];

    storage.readAt(
        "appointments.dat",
        4,
        readAppointment,
        APPOINTMENT_RECORD_SIZE
    );

    check("Test 11 - Appointment record read correctly",
        memcmp(appointmentBuffer, readAppointment,
            APPOINTMENT_RECORD_SIZE) == 0);


    // Read second Appointment
    storage.readAt(
        "appointments.dat",
        64,
        readAppointment,
        APPOINTMENT_RECORD_SIZE
    );

    check("Test 11 - Second Appointment read correctly",
        memcmp(appointmentBuffer, readAppointment,
            APPOINTMENT_RECORD_SIZE) == 0);


    // =====================================
    // TEST 12: Check final file sizes
    // =====================================

    ifstream doctorsSize("doctors.dat", ios::binary | ios::ate);
    ifstream appointmentsSize("appointments.dat", ios::binary | ios::ate);

    check("Test 12 - Doctors file size is 194 bytes",
        doctorsSize.tellg() == 194);

    check("Test 12 - Appointments file size is 124 bytes",
        appointmentsSize.tellg() == 124);

    doctorsSize.close();
    appointmentsSize.close();


    // =====================================
    // TEST 13: Initialize existing files
    // =====================================

    storage.writeHeader("doctors.dat", 4);

    storage.initializeFiles();

    check("Test 13 - Existing header preserved",
        storage.readHeader("doctors.dat") == 4);

    storage.readAt("doctors.dat", 4, readDoctor, DOCTOR_RECORD_SIZE);

    check("Test 13 - Existing record preserved",
        memcmp(doctorBuffer, readDoctor, DOCTOR_RECORD_SIZE) == 0);

   // Prepare unique records for Primary Index testing
   // Doctors: 5 records
char doctorData[DOCTOR_RECORD_SIZE];

storage.saveDoctor("D001", "Ahmed Ali", "Cardiology", "Cairo", doctorData);
storage.writeDoctorRecord(0, doctorData);

storage.saveDoctor("D002", "Mona Samir", "Neurology", "Giza", doctorData);
storage.writeDoctorRecord(1, doctorData);

storage.saveDoctor("D003", "Omar Adel", "Surgery", "Cairo", doctorData);
storage.writeDoctorRecord(2, doctorData);

storage.saveDoctor("D004", "Sara Hany", "Pediatrics", "Giza", doctorData);
storage.writeDoctorRecord(3, doctorData);

storage.saveDoctor("D005", "Ali Hassan", "Dermatology", "Cairo", doctorData);
storage.writeDoctorRecord(4, doctorData);


// Appointments: 10 records
char appointmentData[APPOINTMENT_RECORD_SIZE];

storage.saveAppointment("A001", "2026-11-05", "D001", appointmentData);
storage.writeAppointmentRecord(0, appointmentData);

storage.saveAppointment("A002", "2026-11-06", "D002", appointmentData);
storage.writeAppointmentRecord(1, appointmentData);

storage.saveAppointment("A003", "2026-11-07", "D003", appointmentData);
storage.writeAppointmentRecord(2, appointmentData);

storage.saveAppointment("A004", "2026-11-08", "D004", appointmentData);
storage.writeAppointmentRecord(3, appointmentData);

storage.saveAppointment("A005", "2026-11-09", "D005", appointmentData);
storage.writeAppointmentRecord(4, appointmentData);

storage.saveAppointment("A006", "2026-11-10", "D001", appointmentData);
storage.writeAppointmentRecord(5, appointmentData);

storage.saveAppointment("A007", "2026-11-11", "D002", appointmentData);
storage.writeAppointmentRecord(6, appointmentData);

storage.saveAppointment("A008", "2026-11-12", "D003", appointmentData);
storage.writeAppointmentRecord(7, appointmentData);

storage.saveAppointment("A009", "2026-11-13", "D004", appointmentData);
storage.writeAppointmentRecord(8, appointmentData);

storage.saveAppointment("A010", "2026-11-14", "D005", appointmentData);
storage.writeAppointmentRecord(9, appointmentData); 


    cout << "\nAll tests completed!" << endl;

// Primary index test

cout << "PRIMARY INDEX TESTS \n";

PrimaryIndex primary(storage);

// Test 1: Build indexes from the existing data files
check("Build indexes", primary.buildIndexes());

// Test 2: Check number of records in each index
check("Doctors index size",
      primary.getDoctorsIndex().size() == 5);

check("Appointments index size",
      primary.getAppointmentsIndex().size() == 10);

// Test 3: Search for existing records
check("Find doctor D003",
      primary.findDoctor("D003") != -1);

check("Find appointment A005",
      primary.findAppointment("A005") != -1);

// Test 4: Search for missing records
check("Find missing doctor",
      primary.findDoctor("D999") == -1);

check("Find missing appointment",
      primary.findAppointment("A999") == -1);

// Test 5: Check returned offsets
int doctorPosition = primary.findDoctor("D003");

check("Doctor D003 offset",
      doctorPosition != -1 &&
      primary.getDoctorsIndex()[doctorPosition].offset == 194);

int appointmentPosition = primary.findAppointment("A005");

check("Appointment A005 offset",
      appointmentPosition != -1 &&
      primary.getAppointmentsIndex()[appointmentPosition].offset == 244);

// Test 6: Add new entries
check("Add doctor D006",
      primary.addDoctor("D006", 479));

check("Add appointment A011",
      primary.addAppointment("A011", 604));

// Test 7: Prevent duplicate keys
check("Reject duplicate doctor",
      !primary.addDoctor("D001", 574));

check("Reject duplicate appointment",
      !primary.addAppointment("A001", 664));

// Test 8: Delete entries
check("Delete doctor D002",
      primary.deleteDoctor("D002"));

check("Delete appointment A003",
      primary.deleteAppointment("A003"));

check("Deleted doctor not found",
      primary.findDoctor("D002") == -1);

check("Deleted appointment not found",
      primary.findAppointment("A003") == -1);

// Test 9: Update primary keys
check("Update doctor ID",
      primary.updateDoctorID("D003", "D000"));

check("Update appointment ID",
      primary.updateAppointmentID("A005", "A000"));

check("Old doctor ID not found",
      primary.findDoctor("D003") == -1);

check("New doctor ID found",
      primary.findDoctor("D000") != -1);

check("Old appointment ID not found",
      primary.findAppointment("A005") == -1);

check("New appointment ID found",
      primary.findAppointment("A000") != -1);

// Test 10: Print final indexes
primary.printIndexes();


    return 0;
}

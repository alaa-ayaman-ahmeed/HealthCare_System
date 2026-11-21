
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>

#include "StorageManager.h"
#include "PrimaryIndex.h"
#include "SecondaryIndex.h"
#include "OperationsManager.h"

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

    check("Test 8 - Doctor 1 written at offset 4", true);
    check("Test 8 - Doctor 2 written at offset 99", true);

    // =====================================
    // TEST 9: Write Appointment using RRN
    // =====================================

    storage.writeAppointmentRecord(0, appointmentBuffer);
    storage.writeAppointmentRecord(1, appointmentBuffer);

    check("Test 9 - Appointment 1 written at offset 4", true);
    check("Test 9 - Appointment 2 written at offset 64", true);

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

    // =====================================
    // PREPARE DATA FOR INDEX TESTING
    // =====================================

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

    cout << "\nAll initial tests completed!\n";

    // =====================================
    // PRIMARY INDEX TESTS
    // =====================================

    cout << "\nPRIMARY INDEX TESTS\n";

    PrimaryIndex primary(storage);

    check("Build indexes", primary.buildIndexes());

    check("Doctors index size",
        primary.getDoctorsIndex().size() == 5);

    check("Appointments index size",
        primary.getAppointmentsIndex().size() == 10);

    check("Find doctor D003",
        primary.findDoctor("D003") != -1);

    check("Find appointment A005",
        primary.findAppointment("A005") != -1);

    check("Find missing doctor",
        primary.findDoctor("D999") == -1);

    check("Find missing appointment",
        primary.findAppointment("A999") == -1);

    int doctorPosition = primary.findDoctor("D003");

    check("Doctor D003 offset",
        doctorPosition != -1 &&
        primary.getDoctorsIndex()[doctorPosition].offset == 194);

    int appointmentPosition = primary.findAppointment("A005");

    check("Appointment A005 offset",
        appointmentPosition != -1 &&
        primary.getAppointmentsIndex()[appointmentPosition].offset == 244);

    check("Add doctor D006",
        primary.addDoctor("D006", 479));

    check("Add appointment A011",
        primary.addAppointment("A011", 604));

    check("Reject duplicate doctor",
        !primary.addDoctor("D001", 574));

    check("Reject duplicate appointment",
        !primary.addAppointment("A001", 664));

    check("Delete doctor D002",
        primary.deleteDoctor("D002"));

    check("Delete appointment A003",
        primary.deleteAppointment("A003"));

    check("Deleted doctor not found",
        primary.findDoctor("D002") == -1);

    check("Deleted appointment not found",
        primary.findAppointment("A003") == -1);

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

    primary.printIndexes();

    // =====================================
    // SECONDARY INDEX TESTS
    // =====================================

    cout << "\nSECONDARY INDEX TESTS\n";

    PrimaryIndex prim(storage);
    prim.buildIndexes();

    SecondaryIndex docNameIdx("docname_keys.idx", "docname_list.dat", 30);
    SecondaryIndex docSpecIdx("docspec_keys.idx", "docspec_list.dat", 20);
    SecondaryIndex appDocIdx("appdoc_keys.idx", "appdoc_list.dat", 15);

    docNameIdx.clear();
    docSpecIdx.clear();
    appDocIdx.clear();

    for (const auto& e : prim.getDoctorsIndex()) {
        char rec[DOCTOR_RECORD_SIZE];

        storage.readAt("doctors.dat", e.offset, rec, DOCTOR_RECORD_SIZE);

        docNameIdx.add(string(rec + 15, 30), e.key);
        docSpecIdx.add(string(rec + 45, 20), e.key);
    }

    for (const auto& e : prim.getAppointmentsIndex()) {
        char rec[APPOINTMENT_RECORD_SIZE];

        storage.readAt("appointments.dat", e.offset, rec, APPOINTMENT_RECORD_SIZE);

        appDocIdx.add(string(rec + 45, 15), e.key);
    }

    check("Sec - search doctor name",
        docNameIdx.search("Ahmed Ali") == vector<string>{"D001"});

    check("Sec - search specialty",
        docSpecIdx.search("Cardiology") == vector<string>{"D001"});

    check("Sec - appointments of D001",
        appDocIdx.search("D001") == vector<string>{"A001", "A006"});

    check("Sec - missing key is empty",
        appDocIdx.search("D999").empty());

    cout << "Appointments of D001:\n";

    for (const string& id : appDocIdx.search("D001")) {
        int pos = prim.findAppointment(id);
        int off = prim.getAppointmentsIndex()[pos].offset;

        char rec[APPOINTMENT_RECORD_SIZE];

        storage.readAt("appointments.dat", off, rec, APPOINTMENT_RECORD_SIZE);

        cout << "  " << id << " at offset " << off
             << " date " << string(rec + 15, 30) << endl;
    }

    vector<string> byName = docNameIdx.search("Ahmed Ali");
    vector<string> bySpec = docSpecIdx.search("Cardiology");
    vector<string> both;

    set_intersection(
        byName.begin(), byName.end(),
        bySpec.begin(), bySpec.end(),
        back_inserter(both)
    );

    check("Sec - name AND specialty query",
        both == vector<string>{"D001"});

    // Update Doctor ID D003 -> D000 in the index tests
    prim.updateDoctorID("D003", "D000");

    docNameIdx.replacePrimary("Omar Adel", "D003", "D000");
    docSpecIdx.replacePrimary("Surgery", "D003", "D000");
    appDocIdx.renameKey("D003", "D000");

    check("Sec - ID update in name index",
        docNameIdx.search("Omar Adel") == vector<string>{"D000"});

    check("Sec - appointments moved to new doctor ID",
        appDocIdx.search("D000") == vector<string>{"A003", "A008"});

    check("Sec - old doctor ID has no appointments",
        appDocIdx.search("D003").empty());

    docNameIdx.remove("Mona Samir", "D002");
    docSpecIdx.remove("Neurology", "D002");

    check("Sec - deleted doctor gone from name index",
        docNameIdx.search("Mona Samir").empty());

    check("Sec - deleted doctor gone from specialty index",
        docSpecIdx.search("Neurology").empty());

    docNameIdx.print();
    docSpecIdx.print();
    appDocIdx.print();

    // =====================================
    // OPERATIONS MANAGER TESTS
    // =====================================

    cout << "\nOPERATIONS MANAGER TESTS\n";

    OperationsManager operations(
        storage,
        prim,
        docNameIdx,
        docSpecIdx,
        appDocIdx
    );

    // The previous Secondary Index test changed D003 to D000
    // in the indexes only. Synchronize the data file as well.
    check("Restore doctor ID consistency",
        operations.updateDoctorID("D000", "D003"));

    // -------------------------------------
    // ADD TESTS
    // -------------------------------------

    cout << "\nADD TESTS\n";

    check("Add new doctor D006",
        operations.addDoctor(
            "D006",
            "Khaled Mostafa",
            "Ophthalmology",
            "Cairo"
        ));

    check("New doctor exists in primary index",
        prim.findDoctor("D006") != -1);

    check("New doctor exists in name index",
        docNameIdx.search("Khaled Mostafa") == vector<string>{"D006"});

    check("New doctor exists in specialty index",
        docSpecIdx.search("Ophthalmology") == vector<string>{"D006"});

    check("Reject duplicate doctor ID",
        !operations.addDoctor(
            "D006",
            "Another Doctor",
            "Surgery",
            "Giza"
        ));

    check("Reject invalid doctor data",
        !operations.addDoctor(
            "D007",
            "",
            "Surgery",
            "Cairo"
        ));

    check("Add appointment A011",
        operations.addAppointment(
            "A011",
            "2026-11-15",
            "D006"
        ));

    check("New appointment exists in primary index",
        prim.findAppointment("A011") != -1);

    check("New appointment exists in secondary index",
        appDocIdx.search("D006") == vector<string>{"A011"});

    check("Reject appointment with missing doctor",
        !operations.addAppointment(
            "A012",
            "2026-11-16",
            "D999"
        ));

    check("Reject duplicate appointment ID",
        !operations.addAppointment(
            "A011",
            "2026-11-17",
            "D006"
        ));

    // -------------------------------------
    // UPDATE TESTS
    // -------------------------------------

    cout << "\nUPDATE TESTS\n";

    check("Update doctor name",
        operations.updateDoctorName("D006", "Khaled Hassan"));

    check("Old doctor name removed from index",
        docNameIdx.search("Khaled Mostafa").empty());

    check("New doctor name exists in index",
        docNameIdx.search("Khaled Hassan") == vector<string>{"D006"});

    check("Update appointment date",
        operations.updateAppointmentDate("A011", "2026-12-01"));

    char updatedAppointment[APPOINTMENT_RECORD_SIZE];

    int updatedAppointmentPos = prim.findAppointment("A011");

    int updatedAppointmentOffset =
        prim.getAppointmentsIndex()[updatedAppointmentPos].offset;

    storage.readAt(
        "appointments.dat",
        updatedAppointmentOffset,
        updatedAppointment,
        APPOINTMENT_RECORD_SIZE
    );

    check("Appointment date changed in file",
        string(updatedAppointment + 15, 30) ==
        "2026-12-01                    ");

    check("Update doctor ID D006 to D007",
        operations.updateDoctorID("D006", "D007"));

    check("Old doctor ID no longer exists",
        prim.findDoctor("D006") == -1);

    check("New doctor ID exists",
        prim.findDoctor("D007") != -1);

    check("Doctor name index updated after ID change",
        docNameIdx.search("Khaled Hassan") == vector<string>{"D007"});

    check("Doctor specialty index updated after ID change",
        docSpecIdx.search("Ophthalmology") == vector<string>{"D007"});

    check("Appointment index updated after doctor ID change",
        appDocIdx.search("D007") == vector<string>{"A011"});

    // -------------------------------------
    // DELETE TESTS
    // -------------------------------------

    cout << "\nDELETE TESTS\n";

    // Doctor cannot be deleted while appointments still exist.
    check("Reject deleting doctor with appointments",
        !operations.deleteDoctor("D007"));

    check("Delete appointment A011",
        operations.deleteAppointment("A011"));

    check("Deleted appointment removed from primary index",
        prim.findAppointment("A011") == -1);

    check("Deleted appointment removed from secondary index",
        appDocIdx.search("D007").empty());

    check("Delete doctor D007",
        operations.deleteDoctor("D007"));

    check("Deleted doctor removed from primary index",
        prim.findDoctor("D007") == -1);

    check("Deleted doctor removed from name index",
        docNameIdx.search("Khaled Hassan").empty());

    check("Deleted doctor removed from specialty index",
        docSpecIdx.search("Ophthalmology").empty());

    // -------------------------------------
    // AVAIL LIST REUSE TESTS
    // -------------------------------------

    cout << "\nAVAIL LIST REUSE TESTS\n";

    check("Add doctor D008 using available record",
        operations.addDoctor(
            "D008",
            "Nour Ahmed",
            "Dentistry",
            "Giza"
        ));

    check("Add appointment A012 using available record",
        operations.addAppointment(
            "A012",
            "2026-12-05",
            "D008"
        ));

    check("Reused doctor record is indexed",
        prim.findDoctor("D008") != -1);

    check("Reused appointment record is indexed",
        prim.findAppointment("A012") != -1);

    check("Delete reused appointment A012",
        operations.deleteAppointment("A012"));

    check("Delete reused doctor D008",
        operations.deleteDoctor("D008"));

    cout << "\nALL OPERATIONS MANAGER TESTS COMPLETED!\n";

    return 0;
}


#include "QueryEngine.h"

#include <algorithm>
#include <cctype>
#include <sstream>


// =====================================
// Constructor
// =====================================

QueryEngine::QueryEngine(
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
      appDocIdx(appIdx)
{
}


// =====================================
// Clean Fixed-Length Fields
// =====================================

string QueryEngine::cleanField(const char* data, int size) const {

    string result(data, size);

    while (!result.empty() &&
           (result.back() == ' ' || result.back() == '\0')) {

        result.pop_back();
    }

    return result;
}


// =====================================
// Convert String to Uppercase
// =====================================

string QueryEngine::toUpper(string text) const {

    transform(
        text.begin(),
        text.end(),
        text.begin(),
        [](unsigned char c) {
            return static_cast<char>(toupper(c));
        }
    );

    return text;
}


// =====================================
// Remove Extra Spaces
// =====================================

string QueryEngine::trim(string text) const {

    size_t start = text.find_first_not_of(" \t\r\n");

    if (start == string::npos)
        return "";

    size_t end = text.find_last_not_of(" \t\r\n");

    return text.substr(start, end - start + 1);
}


// =====================================
// Split WHERE Conditions Using AND
// =====================================

vector<string> QueryEngine::splitConditions(
    const string& wherePart
) const {

    vector<string> conditions;

    string upperPart = toUpper(wherePart);

    size_t start = 0;
    size_t position;

    while ((position = upperPart.find(" AND ", start))
           != string::npos) {

        conditions.push_back(
            trim(wherePart.substr(start, position - start))
        );

        start = position + 5;
    }

    conditions.push_back(trim(wherePart.substr(start)));

    return conditions;
}


// =====================================
// Parse One Condition
// =====================================

bool QueryEngine::parseCondition(
    const string& condition,
    string& field,
    string& value
) const {

    size_t equalPosition = condition.find('=');

    if (equalPosition == string::npos)
        return false;


    field = trim(condition.substr(0, equalPosition));

    value = trim(condition.substr(equalPosition + 1));


    // Normalize field name:
    // "Doctor ID" becomes "DOCTORID"
    // "DoctorID" also becomes "DOCTORID"

    field = toUpper(field);

    field.erase(
        remove(field.begin(), field.end(), ' '),
        field.end()
    );


    // Remove quotes from the value

    if (value.size() >= 2 &&
        ((value.front() == '\'' && value.back() == '\'') ||
         (value.front() == '"' && value.back() == '"'))) {

        value = value.substr(1, value.size() - 2);
    }


    value = trim(value);

    return !field.empty() && !value.empty();
}


// =====================================
// Intersect Two Lists of Primary Keys
// =====================================

vector<string> QueryEngine::intersect(
    const vector<string>& first,
    const vector<string>& second
) const {

    vector<string> result;

    for (const string& id : first) {

        if (find(second.begin(), second.end(), id)
            != second.end()) {

            result.push_back(id);
        }
    }

    return result;
}


// =====================================
// Get Doctor Record Using Primary Index
// =====================================

bool QueryEngine::getDoctorRecord(
    const string& id,
    DoctorRecord& doctor
) const {

    int position = primary.findDoctor(id);

    if (position == -1)
        return false;


    int offset =
        primary.getDoctorsIndex()[position].offset;


    storage.readAt(
        "doctors.dat",
        offset,
        reinterpret_cast<char*>(&doctor),
        DOCTOR_RECORD_SIZE
    );

    return true;
}


// =====================================
// Get Appointment Record Using Primary Index
// =====================================

bool QueryEngine::getAppointmentRecord(
    const string& id,
    AppointmentRecord& appointment
) const {

    int position = primary.findAppointment(id);

    if (position == -1)
        return false;


    int offset =
        primary.getAppointmentsIndex()[position].offset;


    storage.readAt(
        "appointments.dat",
        offset,
        reinterpret_cast<char*>(&appointment),
        APPOINTMENT_RECORD_SIZE
    );

    return true;
}


// =====================================
// Print Doctor Fields
// =====================================

void QueryEngine::printDoctor(
    const DoctorRecord& doctor,
    const string& selectedFields
) const {

    vector<string> fields;

    if (trim(selectedFields) == "*") {

        fields = {
            "DOCTORID",
            "DOCTORNAME",
            "DOCTORSPECIALTY",
            "ADDRESS"
        };
    }

    else {

        stringstream ss(selectedFields);
        string field;

        while (getline(ss, field, ',')) {

            field = toUpper(trim(field));

            field.erase(
                remove(field.begin(), field.end(), ' '),
                field.end()
            );

            fields.push_back(field);
        }
    }


    for (const string& field : fields) {

        if (field == "DOCTORID") {

            cout << "Doctor ID: "
                 << cleanField(doctor.doctorID, 15)
                 << endl;
        }

        else if (field == "DOCTORNAME") {

            cout << "Doctor Name: "
                 << cleanField(doctor.doctorName, 30)
                 << endl;
        }

        else if (field == "DOCTORSPECIALTY") {

            cout << "Doctor Specialty: "
                 << cleanField(doctor.doctorSpecialty, 20)
                 << endl;
        }

        else if (field == "ADDRESS") {

            cout << "Address: "
                 << cleanField(doctor.address, 30)
                 << endl;
        }
    }

    cout << "----------------------------------\n";
}


// =====================================
// Print Appointment Fields
// =====================================

void QueryEngine::printAppointment(
    const AppointmentRecord& appointment,
    const string& selectedFields
) const {

    vector<string> fields;

    if (trim(selectedFields) == "*") {

        fields = {
            "APPOINTMENTID",
            "APPOINTMENTDATE",
            "DOCTORID"
        };
    }

    else {

        stringstream ss(selectedFields);
        string field;

        while (getline(ss, field, ',')) {

            field = toUpper(trim(field));

            field.erase(
                remove(field.begin(), field.end(), ' '),
                field.end()
            );

            fields.push_back(field);
        }
    }


    for (const string& field : fields) {

        if (field == "APPOINTMENTID") {

            cout << "Appointment ID: "
                 << cleanField(appointment.appointmentID, 15)
                 << endl;
        }

        else if (field == "APPOINTMENTDATE") {

            cout << "Appointment Date: "
                 << cleanField(appointment.appointmentDate, 30)
                 << endl;
        }

        else if (field == "DOCTORID") {

            cout << "Doctor ID: "
                 << cleanField(appointment.doctorID, 15)
                 << endl;
        }
    }

    cout << "----------------------------------\n";
}


// =====================================
// Execute Query
// =====================================

bool QueryEngine::executeQuery(const string& query) {

    string cleanedQuery = trim(query);


    // Remove semicolon if it exists

    if (!cleanedQuery.empty() &&
        cleanedQuery.back() == ';') {

        cleanedQuery.pop_back();
    }


    string upperQuery = toUpper(cleanedQuery);


    // ---------------------------------
    // Check SELECT
    // ---------------------------------

    if (upperQuery.find("SELECT ") != 0) {

        cout << "Invalid Query: SELECT is required.\n";
        return false;
    }


    // ---------------------------------
    // Find FROM
    // ---------------------------------

    size_t fromPosition = upperQuery.find(" FROM ");

    if (fromPosition == string::npos) {

        cout << "Invalid Query: FROM is missing.\n";
        return false;
    }


    string selectedFields =
        trim(cleanedQuery.substr(7, fromPosition - 7));


    // ---------------------------------
    // Find WHERE
    // ---------------------------------

    size_t wherePosition =
        upperQuery.find(" WHERE ", fromPosition + 6);


    if (wherePosition == string::npos) {

        cout << "Invalid Query: WHERE is missing.\n";
        return false;
    }


    string tableName = trim(
        cleanedQuery.substr(
            fromPosition + 6,
            wherePosition - (fromPosition + 6)
        )
    );

    tableName = toUpper(tableName);


    string wherePart =
        trim(cleanedQuery.substr(wherePosition + 7));


    if (selectedFields.empty() || wherePart.empty()) {

        cout << "Invalid Query.\n";
        return false;
    }


    // ---------------------------------
    // Parse Conditions
    // ---------------------------------

    vector<string> conditions =
        splitConditions(wherePart);


    if (conditions.empty() || conditions.size() > 2) {

        cout << "Invalid Query: Only one or two conditions are supported.\n";
        return false;
    }


    vector<string> fields;
    vector<string> values;


    for (const string& condition : conditions) {

        string field, value;

        if (!parseCondition(condition, field, value)) {

            cout << "Invalid WHERE condition.\n";
            return false;
        }

        fields.push_back(field);
        values.push_back(value);
    }


    vector<string> resultIDs;


    // =================================
    // DOCTORS TABLE
    // =================================

    if (tableName == "DOCTORS") {


        // ---------------------------------
        // Doctor ID: Primary Index
        // ---------------------------------

        if (fields.size() == 1 &&
            fields[0] == "DOCTORID") {

            int position = primary.findDoctor(values[0]);

            if (position != -1) {

                resultIDs.push_back(
                    primary.getDoctorsIndex()[position].key
                );
            }
        }


        // ---------------------------------
        // Doctor Name: Secondary Index
        // ---------------------------------

        else if (fields.size() == 1 &&
                 fields[0] == "DOCTORNAME") {

            resultIDs = docNameIdx.search(values[0]);
        }


        // ---------------------------------
        // Doctor Specialty: Secondary Index
        // ---------------------------------

        else if (fields.size() == 1 &&
                 fields[0] == "DOCTORSPECIALTY") {

            resultIDs = docSpecIdx.search(values[0]);
        }


        // ---------------------------------
        // Doctor Name AND Specialty
        // ---------------------------------

        else if (fields.size() == 2) {

            int namePosition = -1;
            int specialtyPosition = -1;


            for (int i = 0; i < 2; i++) {

                if (fields[i] == "DOCTORNAME")
                    namePosition = i;

                else if (fields[i] == "DOCTORSPECIALTY")
                    specialtyPosition = i;
            }


            if (namePosition != -1 &&
                specialtyPosition != -1) {

                vector<string> nameResults =
                    docNameIdx.search(values[namePosition]);

                vector<string> specialtyResults =
                    docSpecIdx.search(values[specialtyPosition]);


                resultIDs = intersect(
                    nameResults,
                    specialtyResults
                );
            }

            else {

                cout << "Unsupported Doctor Query.\n";
                return false;
            }
        }


        else {

            cout << "Unsupported Doctor Query.\n";
            return false;
        }


        // ---------------------------------
        // Display Doctor Results
        // ---------------------------------

        if (resultIDs.empty()) {

            cout << "No Doctors found.\n";
            return true;
        }


        cout << "\nDoctor Search Results:\n";
        cout << "==================================\n";


        for (const string& id : resultIDs) {

            DoctorRecord doctor;

            if (getDoctorRecord(id, doctor)) {

                printDoctor(doctor, selectedFields);
            }
        }

        return true;
    }


    // =================================
    // APPOINTMENTS TABLE
    // =================================

    else if (tableName == "APPOINTMENTS") {


        // ---------------------------------
        // Appointment ID: Primary Index
        // ---------------------------------

        if (fields.size() == 1 &&
            fields[0] == "APPOINTMENTID") {

            int position =
                primary.findAppointment(values[0]);


            if (position != -1) {

                resultIDs.push_back(
                    primary.getAppointmentsIndex()[position].key
                );
            }
        }


        // ---------------------------------
        // Doctor ID: Secondary Index
        // ---------------------------------

        else if (fields.size() == 1 &&
                 fields[0] == "DOCTORID") {

            resultIDs = appDocIdx.search(values[0]);
        }


        else {

            cout << "Unsupported Appointment Query.\n";
            return false;
        }


        // ---------------------------------
        // Display Appointment Results
        // ---------------------------------

        if (resultIDs.empty()) {

            cout << "No Appointments found.\n";
            return true;
        }


        cout << "\nAppointment Search Results:\n";
        cout << "==================================\n";


        for (const string& id : resultIDs) {

            AppointmentRecord appointment;

            if (getAppointmentRecord(id, appointment)) {

                printAppointment(
                    appointment,
                    selectedFields
                );
            }
        }

        return true;
    }


    // =================================
    // Invalid Table
    // =================================

    else {

        cout << "Invalid Table Name.\n";
        return false;
    }
}

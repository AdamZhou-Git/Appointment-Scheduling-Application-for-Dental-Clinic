#Appointment Scheduling Application for Dental Clinic

## About

This is an appointment scheduling and customer management application that allows you to create a public-facing booking page for Dental Clinic. 
The application is designed to include 3 roles, which are customers, owner and dentists. Each role has a set of role-based access controls (RBAC)
to ensure the professional and efficient communications. 

This Application does not have a frontend implemented. It is a server only application at the moment.


##Motivation
Simplify and streamline the daily booking process with the skill set of Flask, SQLAlchemy, PostgreSQL, Postman, Auth0, gunicorn and RESTful API deployment on Heroku. 

## Application Heroku link:

 https://mydentalproject.herokuapp.com/ 

## Instructions to set up authentication

1. Follow below Auth0 url for the login page: https://dev-v6hy9z2c.us.auth0.com/authorize?audience=DZproject&response_type=token&client_id=RtaesOMPljkhXk3JZvKCymuoWGl5lJxz&redirect_uri=http://localhost:8080/login-results

2. Enter the email address and the password for role-based token:

Role | Email | Password
| ------------- |:-------------:| -----:|
| Customer1      | customer1@email.com | customer1@email.com |
| Dentist1     | dentist1@email.com      |   dentist1@email.com |
| Clinic Owner | clinicowner@email.com      |    ClinicOwner@email.com |


##Application Stack

### Tech Stack:
* Python3: The server language
* Flask: Server Framework
* PostgreSQL: Database
* SQLAlchemy: ORM of choice to communicate between the python server and the Postgres Database.
* Heroku: Application Deployment Platform
* Postman: Testing the application API endpoints

###Dependencies/Libraries
The full list of dependencies can be found in the requirement.txt. Some major dependencies are:
* alembic
* Flask
* Flask-Cors
* Flask-Migrate
* Flask-Script
* Flask-SQLAlchemy
* Jinja2
* psycopg2-binary
* Werkzeug
* python-jose
* pycryptodome


## Run the App in Local

1. Clone the project repository
```
git clone https://github.com/AdamZhou-Git/Appointment_Scheduling_Application
```
2. Create a virtual environment
```
python3 -m venv myvenv
source env/Scripts/activate # for windows
source env/bin/activate # for MacOs
```
3. Set up the environment variables
```
python3 -m venv myvenv
source env/Scripts/activate # for windows
source env/bin/activate # for MacOs
```
4. Install dependencies
```
pip install -r requirements.txt
```
5. Create your local database:
```angular2html
flask db init
createdb dbname
dropdb dbname
```
6. Import the sample data (optional):
```angular2html
psql dbname < dental.psql
```
7. Set up the environment variables
```
chmod +x setup.sh
source setup.sh
```
8. Run the application
```angular2html
export FLASK_APP=app.py
export FLASK_ENV=development
flask run
```

##Deploy the App on Heroku
1. Prerequisite: check your heroku and postgres version and login to heroku
```angular2html
postgres --version
heroku --version
heroku login -i
```
2. Initialize Git
```
git init
git config --global user.email "you@example.com"
git config --global user.name "Your Name"
```
Whenever you make any changes to your application folder contents, you will have to run git add and git commit commands.
```angular2html
git add .
git status
git commit -m "your message"
```
3. Create an App in Heroku Cloud
```
heroku create [my-app-name] --buildpack heroku/python
```
where, my-app-name is a unique name that nobody else on Heroku has already used. You have to define the build environment using the option --buildpack heroku/python The heroku create command will create a Git "remote" repository on Heroku cloud and a web address for accessing your web app. You can check that a remote repository was added to your git repository with the following terminal command:
```angular2html
git remote -v
```
4. Add PostgreSQL addon for our database
```angular2html
heroku addons:create heroku-postgresql:hobby-dev --app [my-app-name]
```
In the command above,
heroku-postgresql is the name of the addon that will create an empty Postgres database.
hobby-dev on the other hand specifies the tier of the addon, in this case the free version which has a limit on the amount of data it will store, albeit fairly high.
5. Configure the App and update DATABASE_URL in environment variables:
```angular2html
heroku config --app [my-app-name]
```
6. Push it to Heroku 
```angular2html
git push heroku master
```
7. Run migrations: 
Once your app is deployed, run migrations by running:
```angular2html
python manage.py db init
python manage.py db migrate
python manage.py db upgrade
heroku run python manage.py db upgrade --app [my-app-name]
```
8. Push local database to Heroku(optional)
```angular2html
heroku pg:psql --app [my-app-name] < dental.psql
```


## RBAC credentials and roles
Auth0 was set up to manage role-based access control for three users. The API documentation below describes, among others, by which user the endpoints can be accessed. Access credentials and permissions are handled with JWT tockens which must be included in the request header.

###Permissions: 
1. Public/Customer: 
      1. Post customers: for new customer to register
      2. Post Appointments: for customers to create appointments
      3. Patch customers: update customer info by their own id (own id will be fetched from the front end)
      4. Patch Appointments: update the booked appointment by customer id (own id will be fetched from the front end)
      5. Delete Appointment: delete the booked appointment by customer id (own id will be fetched from the front end)

2. Dentist (employee):
      1. GET customers: get all customers information
      2. GET dentists: get all dentists information
      3. GET appointments: get all appointments
      4. PATCH dentists: update their own information
      5. PATCH appointment: update all appointments (newly created appointments are not assigned with any dentists by default; Dentists can view all unassigned appointments and add their license id to the appointment to confirm the appointment)
        
3. Clinic owner:
        all of the endpoints except for the GET/customers/id as this is specificly for customer to review their own information. Clinic owner can review all customers with GET/customer

### API Endpoints
GET /customers, /dentists, /appointments, /customers/id 
DELETE /customers/id, /dentists/id, /appointments/id
POST /customers, /dentists, /appointments
PATCH /customers/id, /dentists/id, /appointments/id

#### GET '/customers'
* Fetches a paginated list of customers with a maximum of 25 customers per page
* Access Role: Clinic Owner, Dentists
* Sample response:
```angular2html
{
    "customers": [
        {
            "customer_id": 17,
            "email": "NewEmail@jourrapide.com",
            "name": "Dewey J. Morton",
            "phone": "812-586-1272",
            "required_service": "Dental Services"
        },
        {
            "customer_id": 13,
            "email": "SandraCKnox@armyspy.com",
            "name": "Sandra C. Knox",
            "phone": "000-000-0000",
            "required_service": "Orthodontic Services,Dental Services"
        }
    ],
    "success": true,
    "total_customers": 2
```
#### GET '/customers/int:customer_id'
* Fetches specific customer
* Access Role: Customers
* Sample response:
```angular2html
{
    "customer": {
        "customer_id": 13,
        "email": "SandraCKnox@armyspy.com",
        "name": "Sandra C. Knox",
        "phone": "000-000-0000",
        "required_service": "Orthodontic Services,Dental Services"
    },
    "success": true,
    "upcoming_appt": [
        {
            "appt_id": 1,
            "cust_id": 13,
            "license_id": "3",
            "service": "Orthodontic Services",
            "start_time": "Thu, 14 Apr 2022 09:00:00 GMT"
        }
    ]
}
```
#### GET '/dentists'
* Fetches a paginated list of dentists with a maximum of 25 dentists per page
* Access Role: Clinic Owner, Dentists
* Sample response:
```angular2html
{
    "dentists": [
        {
            "gender": "male",
            "license_id": 1,
            "name": "Zhang San",
            "specialty": "Orthodontic Services,Dental Services"
        },
        {
            "gender": "male",
            "license_id": 20,
            "name": "Flora R. Payne",
            "specialty": "Dental Services"
        }
    ],
    "success": true,
    "total_dentists": 2
```
#### GET '/appointments'
* Fetches a paginated list of appointments with a maximum of 25 appointments per page
* Access Role: Clinic Owner, Dentists
* Sample response:
```angular2html
{
    "appointments": [
        {
            "appt_id": 8,
            "cust_id": 13,
            "license_id": "false",
            "service": "Orthodontic Services",
            "start_time": "Sun, 01 May 2022 12:00:00 GMT"
        }
    ],
    "success": true,
    "total_appointments": 1
}
```
#### DELETE '/customers/int:customer_id'
* Delete specific customer
* Access Role: Clinic Owner
* Sample response:
```angular2html
{
    "delete": 20,
    "success": true,
    "total_customers": 6
}
```

#### DELETE '/dentists/int:license_id'
* Delete specific dentist
* Access Role: Clinic Owner
* Sample response:
```angular2html
{
    "delete": 21,
    "success": true,
    "total_dentists": 6
}
```

#### DELETE '/appointments/int:appt_id'
* Delete specific appointment
* Access Role: Clinic Owner, Customer
* Sample response:
```angular2html
{
    "delete": 10,
    "success": true,
    "total_appointments": 7
}
```

#### POST '/customers'
* create a new customer
* Access Role: Clinic Owner, Customer
* Sample response:
```angular2html
{
    "new_customer": {
        "customer_id": 20,
        "email": "fakeemial@email.com",
        "name": "fake customer",
        "phone": "111-111-1111",
        "required_service": "Dental Services"
    },
    "success": true
```

#### POST '/dentists'
* create a new dentist
* Access Role: Clinic Owner
* Sample response:
```angular2html
{
    "new_dentist": {
        "gender": "male",
        "license_id": 21,
        "name": "Jay Zhou",
        "specialty": "Dental Services"
    },
    "success": true
}
```

#### POST '/appointments'
* create a new appointment
* Access Role: Clinic Owner, Customer
* Sample response:
```angular2html
{
    "new_appointment": {
        "appt_id": 10,
        "cust_id": 13,
        "license_id": "false",
        "service": "Orthodontic Services",
        "start_time": "Sat, 14 Jan 2023 14:00:00 GMT"
    },
    "success": true
}
```


#### PATCH '/customers/init:customer_id'
* update the info for specific customer
* Access Role: Clinic Owner, Customer
* Sample response:
```angular2html
{
    "success": true,
    "updated_customer": {
        "customer_id": 17,
        "email": "NewEmail@jourrapide.com",
        "name": "Dewey J. Morton",
        "phone": "812-586-1272",
        "required_service": "Dental Services"
    }
}
```

#### PATCH '/dentists/init:license_id'
* update the info for specific dentist
* Access Role: Clinic Owner, Dentists
* Sample response:
```angular2html
{
    "success": true,
    "updated_dentists": {
        "gender": "female",
        "license_id": 2,
        "name": "Li Si",
        "specialty": "Updated service, Dental Services"
    }
}
```

#### PATCH '/appointments/init:appt_id'
* update the info for specific appointment
* Access Role: Clinic Owner, Customers
* Sample response:
```angular2html
{
    "success": true,
    "updated_appt": {
        "appt_id": 4,
        "cust_id": 16,
        "license_id": "1",
        "service": "Orthodontic Services",
        "start_time": "Tue, 14 Jun 2022 14:00:00 GMT"
    }
}
```

## Testing
The testing of all endpoints was implemented with unittest. Each endpoint can be tested with one success test case and one error test case. RBAC feature can also be tested for company user and candidate user.

All test cases are sorted in test_app.py file in the project root folder.

###Steps for testing

1. create a test database using Psql CLI:
```angular2html
create database testdb
```
2. Update the token for 3 roles in environment file

3. run the test file:
```angular2html
python3 test_app.py
```

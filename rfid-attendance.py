import pandas as pd
import serial
import tkinter as tk
from tkinter import messagebox
import time
from datetime import datetime

# Connect to Arduino (adjust COM port if necessary)
try:
    ser = serial.Serial('COM5', 9600, timeout=1)
except serial.SerialException as e:
    print(f"Error opening serial port: {e}")
    messagebox.showerror("Serial Error", f"Could not open port: {e}")
    ser = None  # Prevent further operations if the serial connection fails

# Initialize empty DataFrame
pro_df = pd.DataFrame(columns=['Name', 'UID'])

auth_log = pd.DataFrame(columns=['Profile', 'Timestamp'])  # Initialize auth_log DataFrame


# Function to read profiles from Arduino
def read_profiles():
    if ser is None:
        print("Serial connection is not available")
        return []

    ser.write(b'r')  # Send 'r' command to Arduino
    profiles = []
    start_marker_found = False
    timeout_start = time.time()
    timeout_duration = 5  # Timeout after 5 seconds

    while True:
        if time.time() - timeout_start > timeout_duration:
            print("Timeout: No complete data received from Arduino")
            break

        if ser.in_waiting > 0:
            line = ser.readline().decode().strip()
            print(f"Received line: '{line}'")

            if 'START_PROFILES' in line:
                start_marker_found = True
                continue
            if 'AUTH_SUCCESS' in line:
                profile_name = line.split(',')[1]  # Extract profile name
                log_authentication(profile_name)  # Log the authentication
                continue
            elif 'END_PROFILES' in line:
                break
            
            if start_marker_found and line:
                profile = line.split(',')
                if len(profile) == 2:  # Ensure correct format
                    profiles.append(profile)
                    print(f"Profile added: {profile}")  # Print added profile for debugging

    if not profiles:
        print("No profiles received from Arduino")
    return profiles

# Function to update a profile on Arduino
def update_profile(index, name, uid):
    if ser is None:
        messagebox.showerror("Serial Error", "No connection to Arduino")
        return

    pro_df.loc[index] = [name, uid]  # Update the profile in the DataFrame
    command = f"u{index},{name},{uid}\n"
    ser.write(command.encode())
    messagebox.showinfo("Success", "Profile updated")

# Function to create a new profile
def create_profile(name, uid):
    global pro_df
    if ser is None:
        messagebox.showerror("Serial Error", "No connection to Arduino")
        return
    
    # Add the new profile to the DataFrame
    new_index = len(pro_df)
    pro_df.loc[new_index] = [name, uid]

    command = f"c{name},{uid}\n"  # 'c' command for creating a profile
    ser.write(command.encode())
    messagebox.showinfo("Success", "Profile created")
    display_profiles()

# Function to delete a profile
def delete_profile(index):
    global pro_df
    if ser is None:
        messagebox.showerror("Serial Error", "No connection to Arduino")
        return
    
    # Delete the profile from the DataFrame
    pro_df.drop(index, inplace=True)
    pro_df.reset_index(drop=True, inplace=True)  # Reindex the DataFrame

    command = f"d{index}\n"  # 'd' command for deleting a profile by index
    ser.write(command.encode())
    messagebox.showinfo("Success", "Profile deleted")
    display_profiles()

# listen for authentication
def listen_for_authentication():
    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode().strip()
            print(f"Received from Arduino: {line}")
            if 'AUTH_SUCCESS' in line:
                profile_name = line.split(',')[1]  # Extract profile name
                log_authentication(profile_name)  # Log the authentication
            elif 'AUTH_FAILED' in line:
                print(f"Authentication failed for UID: {line.split(',')[1]}")

# function to log authentication
def log_authentication(pro_name):
    global auth_log
    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    new_log = pd.DataFrame([[pro_name,timestamp]],columns=['Profile', 'Timestamp'])
    auth_log = pd.concat([auth_log,new_log], ignore_index=True)
    print(auth_log)
    display_auth_log()

# Tkinter GUI setup
root = tk.Tk()
root.title("Student Profiles")
root.geometry('500x300')  # Adjusted window size

# Function to display profiles in the GUI
def display_profiles():
    profiles = read_profiles()    
    print(profiles)  # Debugging output to check if profiles are received
    
    global pro_df    
    pro_df = pd.DataFrame(profiles, columns=["Name", "UID"])    

    for widget in root.winfo_children():
        widget.destroy()  # Clear previous widgets

    # Add Create and Refresh buttons at the top
    tk.Button(root, text="Create New Profile", command=open_create_profile_window).grid(row=0, column=0, padx=5, pady=5)
    tk.Button(root, text="Refresh", command=display_profiles).grid(row=0, column=1, padx=5, pady=5)
    tk.Button(root, text="Show Auth Log", command=display_auth_log).grid(row=0, column=2)

    if pro_df.empty:
        tk.Label(root, text="No profiles found or unable to connect to Arduino").grid(row=1, column=0, columnspan=3)
    else:
        for idx, profile in enumerate(profiles):
            name_label = tk.Label(root, text=f"Name: {profile[0]}")
            name_label.grid(row=idx + 1, column=0)

            uid_label = tk.Label(root, text=f"UID: {profile[1]}")
            uid_label.grid(row=idx + 1, column=1)

            edit_button = tk.Button(root, text="Edit", command=lambda i=idx: edit_profile(i, profiles[i]))
            edit_button.grid(row=idx + 1, column=2)
            
            delete_button = tk.Button(root, text="Delete", command=lambda i=idx: delete_profile(i))
            delete_button.grid(row=idx + 1, column=3)

# Display authentication log
def display_auth_log():
    # auth = listen_for_authentication()
    # global auth_log
    # auth_log = pd.DataFrame(auth, columns=["Intern", "Timestamp"])

    for widget in root.winfo_children():
        widget.destroy() 

    tk.Button(root, text="Back", command=display_profiles).grid(row=0, column=0)
    tk.Label(root,text="Authentication Log").grid(row=0,column=1,columnspan=2)    

    if auth_log.empty:
        tk.Label(root, text="No authentication records yet").grid(row=1, column=0, columnspan=2)
    else:
        for idx, log_entry in auth_log.iterrows():
            name_label = tk.Label(root, text=f"Profile: {log_entry['Profile']}")
            name_label.grid(row=idx + 1, column=0)

            timestamp_label = tk.Label(root, text=f"Timestamp: {log_entry['Timestamp']}")
            timestamp_label.grid(row=idx + 1, column=1)

# Function to open the Edit Profile window
def edit_profile(index, profile):
    edit_window = tk.Toplevel(root)
    edit_window.title("Edit Profile")

    tk.Label(edit_window, text="Name:").grid(row=0, column=0)
    name_entry = tk.Entry(edit_window)
    name_entry.insert(0, profile[0])
    name_entry.grid(row=0, column=1)

    tk.Label(edit_window, text="UID:").grid(row=1, column=0)
    uid_entry = tk.Entry(edit_window)
    uid_entry.insert(0, profile[1])
    uid_entry.grid(row=1, column=1)

    update_button = tk.Button(edit_window, text="Update", command=lambda: update_profile(index, name_entry.get(), uid_entry.get()))
    update_button.grid(row=2, column=0, columnspan=2)

# Function to open the Create Profile window
def open_create_profile_window():
    create_window = tk.Toplevel(root)
    create_window.title("Create Profile")

    tk.Label(create_window, text="Name:").grid(row=0, column=0)
    name_entry = tk.Entry(create_window)
    name_entry.grid(row=0, column=1)

    tk.Label(create_window, text="UID:").grid(row=1, column=0)
    uid_entry = tk.Entry(create_window)
    uid_entry.grid(row=1, column=1)

    create_button = tk.Button(create_window, text="Create", command=lambda: create_profile(name_entry.get(), uid_entry.get()))
    create_button.grid(row=2, column=0, columnspan=2)

# Initial display of profiles
display_profiles()

root.mainloop()

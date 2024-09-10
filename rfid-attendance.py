import serial
import tkinter as tk
from tkinter import messagebox

#initialize serial comms
ser = serial.Serial('COM3',9600,timeout=1)

#fxn to read profiles from arduino
def read_profiles():
    ser.write(b'r') #send 'r' command to arduino
    profiles = []
    while True:
        line = ser.readline().decode().strip()
        if line == '':
            break
        profiles.append(line.split(','))
    return profiles

#Function to update a profile
def update_profile(index,name,uid):
    command = f"u{index},{name},{uid}\n"
    ser.write(command.encode())
    messagebox.showinfo("Success", "Profile updated")

# Tkinter GUI
root = tk.Tk()
root.title("Student Profiles")

def display_profiles():
    profiles = read_profiles()
    for widget in root.winfo_children():
        widget.destroy() # clear the previous widgets

    for idx, profile in enumerate(profiles):
        name_label = tk.Label(root, text=f"Name: {profile[0]}")
        name_label.grid(row=idx,column=0)

        uid_label = tk.Label(root,text=f"UID: {profile[1]}")
        uid_label.grid(row=idx,column=1)

        edit_button =tk.Button(root, text="Edit",command=lambda i=idx: edit_profile(i, profiles[i]))
        edit_button.grid(row=idx, column=2)
    

# Edit profile
def edit_profile(index, profile):
    edit_window = tk.Toplevel(root)
    edit_window.title("Edit Profile")

    tk.Label(edit_window, text="Name:").grid(row=0,column=0)
    name_entry = tk.Entry(edit_window)
    name_entry.insert(0, profile[0])
    name_entry.grid(row=0,column=1)
    
    tk.Label(edit_window, text="UID:").grid(row=1,column=0)
    uid_entry = tk.Entry(edit_window)
    uid_entry.insert(0, profile[1])
    uid_entry.grid(row=1,column=1)

    update_button = tk.Button(edit_window, text="Update", command=lambda: update_profile(index, name_entry.get(), uid_entry.get()))
    update_button.grid(row=2,column=0,columnspan=2)

#Refresh Button
refresh_button = tk.Button(root, text="Refresh", command=display_profiles)
refresh_button.grid(row=0,column=0,columnspan=2)

display_profiles()

root.mainloop()
    
import pathlib
import h5py
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

snapshot_dir = "snapshots"  # Adjust this if your directory name is different

# Convert the map object to a list
snapshot_files = list(pathlib.Path(snapshot_dir).glob("*.h5"))

if len(snapshot_files) == 0:
    raise ValueError(f"No snapshot files found in {snapshot_dir}. Please check the directory.")

# Initialize the plot
fig, ax = plt.subplots(figsize=(6, 4), dpi=100)
line, = ax.plot([], [], label='bmid')
ax.set_xlabel('Time')
ax.set_ylabel('Average bmid')
ax.set_title("Time Evolution of bmid")
ax.legend()

# Function to initialize the plot
def init():
    ax.set_xlim(0, 2000)  # Adjust time axis based on your simulation
    ax.set_ylim(-0.05, 0.05)  # Adjust the y-axis based on expected values
    line.set_data([], [])
    return line,

# Function to update the plot for each frame
def update(frame):
    with h5py.File(snapshot_files[frame], mode='r') as file:
        # Load datasets for each snapshot
        bmid = file['tasks']['bmid']
        t = bmid.dims[0]['sim_time']
        
        # Average over the spatial dimensions (288, 144, 1) to get a 1D array
        bmid_avg = np.mean(bmid[:], axis=(1, 2, 3))
        
        # Update the data for the plot
        line.set_data(t[:], bmid_avg[:])
    return line,

# Create the animation
ani = animation.FuncAnimation(fig, update, frames=len(snapshot_files), init_func=init, blit=True)

# Save the animation as a GIF or MP4
ani.save('bmid_animation.gif', writer='imagemagick', fps=5)
# Alternatively, save as MP4:
# ani.save('bmid_animation.mp4', writer='ffmpeg', fps=5)

plt.show()
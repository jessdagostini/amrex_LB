import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib.colors import LinearSegmentedColormap
import matplotlib.gridspec as gridspec

def hilbert3d(order):
    """Generate 3D Hilbert curve coordinates for the given order."""
    if order < 1:
        raise ValueError("Order must be at least 1")
    
    def transform(pos, orientation, n):
        """Transform coordinates to maintain face-to-face connections."""
        x, y, z = pos
        m = n - 1  # Pre-calculate n-1 for efficiency
        
        # Transformation rules that ensure proper connectivity between subcubes
        transformations = {
            0: lambda: [x, y, z],           # identity
            1: lambda: [z, y, x],           # rotate around y=z diagonal
            2: lambda: [y, z, x],           # rotate around x=z diagonal
            3: lambda: [m-y, z, x],         # rotate and reflect
            4: lambda: [m-z, y, x],         # rotate around x axis
            5: lambda: [m-x, y, z],         # reflect in x
            6: lambda: [x, m-y, z],         # reflect in y
            7: lambda: [x, y, m-z],         # reflect in z
        }
        
        return np.array(transformations.get(orientation, transformations[0])())
    
    def hilbert_curve(n):
        """Generate points for a 3D Hilbert curve."""
        if n == 1:
            return [np.array([0, 0, 0])]
        
        n2 = n // 2  # half size
        
        # Define subcube configurations to ensure proper connectivity
        # Each subcube connects to the next through a shared face
        subcube_configs = [
            [0, 0, 0, 0],      # start at origin (lower left back)
            [0, 0, n2, 1],     # connect through back face
            [0, n2, n2, 2],    # connect through top back
            [0, n2, 0, 3],     # connect through top front
            [n2, n2, 0, 6],    # connect through right top front
            [n2, n2, n2, 4],   # connect through right top back
            [n2, 0, n2, 5],    # connect through right bottom back
            [n2, 0, 0, 7],     # finish at right bottom front
        ]
        
        subcurves = []
        for x, y, z, orientation in subcube_configs:
            # Generate base subcurve
            subcurve = hilbert_curve(n2)
            
            # Transform and position each point
            transformed = []
            for point in subcurve:
                # Apply orientation transformation
                new_point = transform(point, orientation, n2)
                # Translate to correct position
                new_point = new_point + np.array([x, y, z])
                transformed.append(new_point)
            
            subcurves.extend(transformed)
        
        return subcurves
    
    # Calculate the size of the curve (2^order)
    n = 2 ** order
    
    # Generate the curve points
    points = hilbert_curve(n)
    
    # Convert to numpy array for easier manipulation
    return np.array(points)

def plot_hilbert3d(order, save_path_prefix):
    """Plot a 3D Hilbert curve of the given order with multiple views."""
    
    # Generate the curve points
    points = hilbert3d(order)
    
    # Create custom colormap for better visualization
    colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b']
    cmap = LinearSegmentedColormap.from_list('custom', colors, N=len(points))
    
    # Define different viewing angles
    views = [
        (20, 45),   # Isometric view
        (0, 0),     # Front view
        (0, 90),    # Side view
        (90, 0),    # Top view
        (45, 45),   # Another isometric view
    ]
    
    for view_idx, (elev, azim) in enumerate(views):
        # Create figure with proper layout
        fig = plt.figure(figsize=(14, 10))
        gs = gridspec.GridSpec(1, 2, width_ratios=[4, 0.3])
        ax = fig.add_subplot(gs[0], projection='3d')
        
        # Plot the curve segments with color progression
        segments = np.stack([points[:-1], points[1:]], axis=1)
        t = np.linspace(0, 1, len(points))
        
        for i, (start, end) in enumerate(segments):
            color = cmap(t[i])
            ax.plot([start[0], end[0]], [start[1], end[1]], [start[2], end[2]],
                   color=color, linewidth=2)
        
        # Add points at the vertices
        scatter = ax.scatter(points[:, 0], points[:, 1], points[:, 2],
                           c=t, cmap=cmap, s=30)
        
        # Set equal aspect ratio
        max_range = np.array([
            points[:, 0].max() - points[:, 0].min(),
            points[:, 1].max() - points[:, 1].min(),
            points[:, 2].max() - points[:, 2].min()
        ]).max() / 2.0
        
        mid_x = (points[:, 0].max() + points[:, 0].min()) * 0.5
        mid_y = (points[:, 1].max() + points[:, 1].min()) * 0.5
        mid_z = (points[:, 2].max() + points[:, 2].min()) * 0.5
        
        ax.set_xlim(mid_x - max_range, mid_x + max_range)
        ax.set_ylim(mid_y - max_range, mid_y + max_range)
        ax.set_zlim(mid_z - max_range, mid_z + max_range)
        
        # Add labels and title
        ax.set_xlabel('X')
        ax.set_ylabel('Y')
        ax.set_zlabel('Z')
        view_name = ['Isometric', 'Front', 'Side', 'Top', 'Alt Isometric'][view_idx]
        ax.set_title(f'3D Hilbert Curve (Order {order}) - {view_name} View')
        
        # Set the viewing angle
        ax.view_init(elev=elev, azim=azim)

        # Add colorbar
        cax = fig.add_subplot(gs[1])
        plt.colorbar(scatter, cax=cax, label='Position along curve')

        # Adjust layout
        plt.tight_layout()

        # Save the figure
        plt.savefig(f'{save_path_prefix}_order{order}_view{view_idx}.png',
                    dpi=300, bbox_inches='tight')
        plt.close()

def main():
    """Generate and save visualizations for different orders."""
    save_path_prefix = 'hilbert3d_corrected'
    for order in [1, 2, 3]:
        print(f"Generating order {order} Hilbert curve visualizations...")
        plot_hilbert3d(order, save_path_prefix)
        print(f"Saved visualizations for order {order}")

if __name__ == "__main__":
    main()
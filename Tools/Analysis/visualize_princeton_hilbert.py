import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from mpl_toolkits.mplot3d.art3d import Line3DCollection
import matplotlib.colors as colors
import argparse

def sgn(x):
    """Return the sign of x: -1 if x < 0, 1 if x > 0, 0 if x == 0."""
    return -1 if x < 0 else (1 if x > 0 else 0)

def generate3d(x, y, z, ax, ay, az, bx, by, bz, cx, cy, cz):
    """Core recursive function for 3D Hilbert curve generation."""
    w = abs(ax + ay + az)
    h = abs(bx + by + bz)
    d = abs(cx + cy + cz)

    (dax, day, daz) = (sgn(ax), sgn(ay), sgn(az))  # unit major direction ("right")
    (dbx, dby, dbz) = (sgn(bx), sgn(by), sgn(bz))  # unit ortho direction ("forward")
    (dcx, dcy, dcz) = (sgn(cx), sgn(cy), sgn(cz))  # unit ortho direction ("up")

    # trivial row/column fills
    if h == 1 and d == 1:
        for i in range(0, w):
            yield(x, y, z)
            (x, y, z) = (x + dax, y + day, z + daz)
        return

    if w == 1 and d == 1:
        for i in range(0, h):
            yield(x, y, z)
            (x, y, z) = (x + dbx, y + dby, z + dbz)
        return

    if w == 1 and h == 1:
        for i in range(0, d):
            yield(x, y, z)
            (x, y, z) = (x + dcx, y + dcy, z + dcz)
        return

    (ax2, ay2, az2) = (ax//2, ay//2, az//2)
    (bx2, by2, bz2) = (bx//2, by//2, bz//2)
    (cx2, cy2, cz2) = (cx//2, cy//2, cz//2)

    w2 = abs(ax2 + ay2 + az2)
    h2 = abs(bx2 + by2 + bz2)
    d2 = abs(cx2 + cy2 + cz2)

    # prefer even steps
    if (w2 % 2) and (w > 2):
        (ax2, ay2, az2) = (ax2 + dax, ay2 + day, az2 + daz)

    if (h2 % 2) and (h > 2):
        (bx2, by2, bz2) = (bx2 + dbx, by2 + dby, bz2 + dbz)

    if (d2 % 2) and (d > 2):
        (cx2, cy2, cz2) = (cx2 + dcx, cy2 + dcy, cz2 + dcz)

    # wide case, split in w only
    if (2*w > 3*h) and (2*w > 3*d):
        yield from generate3d(x, y, z,
                            ax2, ay2, az2,
                            bx, by, bz,
                            cx, cy, cz)

        yield from generate3d(x+ax2, y+ay2, z+az2,
                            ax-ax2, ay-ay2, az-az2,
                            bx, by, bz,
                            cx, cy, cz)

    # do not split in d
    elif 3*h > 4*d:
        yield from generate3d(x, y, z,
                            bx2, by2, bz2,
                            cx, cy, cz,
                            ax2, ay2, az2)

        yield from generate3d(x+bx2, y+by2, z+bz2,
                            ax, ay, az,
                            bx-bx2, by-by2, bz-bz2,
                            cx, cy, cz)

        yield from generate3d(x+(ax-dax)+(bx2-dbx),
                            y+(ay-day)+(by2-dby),
                            z+(az-daz)+(bz2-dbz),
                            -bx2, -by2, -bz2,
                            cx, cy, cz,
                            -(ax-ax2), -(ay-ay2), -(az-az2))

    # do not split in h
    elif 3*d > 4*h:
        yield from generate3d(x, y, z,
                            cx2, cy2, cz2,
                            ax2, ay2, az2,
                            bx, by, bz)

        yield from generate3d(x+cx2, y+cy2, z+cz2,
                            ax, ay, az,
                            bx, by, bz,
                            cx-cx2, cy-cy2, cz-cz2)

        yield from generate3d(x+(ax-dax)+(cx2-dcx),
                            y+(ay-day)+(cy2-dcy),
                            z+(az-daz)+(cz2-dcz),
                            -cx2, -cy2, -cz2,
                            -(ax-ax2), -(ay-ay2), -(az-az2),
                            bx, by, bz)

    # regular case, split in all w/h/d
    else:
        yield from generate3d(x, y, z,
                            bx2, by2, bz2,
                            cx2, cy2, cz2,
                            ax2, ay2, az2)

        yield from generate3d(x+bx2, y+by2, z+bz2,
                            cx, cy, cz,
                            ax2, ay2, az2,
                            bx-bx2, by-by2, bz-bz2)

        yield from generate3d(x+(bx2-dbx)+(cx-dcx),
                            y+(by2-dby)+(cy-dcy),
                            z+(bz2-dbz)+(cz-dcz),
                            ax, ay, az,
                            -bx2, -by2, -bz2,
                            -(cx-cx2), -(cy-cy2), -(cz-cz2))

        yield from generate3d(x+(ax-dax)+bx2+(cx-dcx),
                            y+(ay-day)+by2+(cy-dcy),
                            z+(az-daz)+bz2+(cz-dcz),
                            -cx, -cy, -cz,
                            -(ax-ax2), -(ay-ay2), -(az-az2),
                            bx-bx2, by-by2, bz-bz2)

        yield from generate3d(x+(ax-dax)+(bx2-dbx),
                            y+(ay-day)+(by2-dby),
                            z+(az-daz)+(bz2-dbz),
                            -bx2, -by2, -bz2,
                            cx2, cy2, cz2,
                            -(ax-ax2), -(ay-ay2), -(az-az2))

def gilbert3d(width, height, depth):
    """
    Generate points for a 3D Hilbert curve to fill a cuboid of given dimensions.
    """
    if width >= height and width >= depth:
        yield from generate3d(0, 0, 0,
                            width, 0, 0,
                            0, height, 0,
                            0, 0, depth)
    elif height >= width and height >= depth:
        yield from generate3d(0, 0, 0,
                            0, height, 0,
                            width, 0, 0,
                            0, 0, depth)
    else:  # depth >= width and depth >= height
        yield from generate3d(0, 0, 0,
                            0, 0, depth,
                            width, 0, 0,
                            0, height, 0)

def plot_hilbert3d_princeton(width, height, depth, save_path=None):
    """Create a visualization of the 3D Hilbert curve with multiple views."""
    # Generate curve points
    points = np.array(list(gilbert3d(width, height, depth)))
    
    # Define viewing angles for different perspectives
    views = [
        (45, 45),   # Isometric view
        (0, 0),     # Front view
        (0, 90),    # Side view
        (90, 0),    # Top view
    ]
    
    # Create figure with subplots for different views
    fig = plt.figure(figsize=(20, 5))
    
    for idx, (elev, azim) in enumerate(views, 1):
        ax = fig.add_subplot(1, 4, idx, projection='3d')
        
        # Create line segments
        segments = np.concatenate([points[:-1, None, :], points[1:, None, :]], axis=1)
        
        # Create color gradient along the curve
        t = np.linspace(0, 1, len(points)-1)
        colors_array = plt.cm.viridis(t)
        
        # Plot line segments with color gradient
        lc = Line3DCollection(segments, colors=colors_array)
        ax.add_collection3d(lc)
        
        # Plot points
        scatter = ax.scatter(points[:, 0], points[:, 1], points[:, 2],
                           c=np.linspace(0, 1, len(points)),
                           cmap='viridis', s=20)
        
        # Add arrows at regular intervals to show direction
        arrow_indices = np.linspace(0, len(points)-2, min(20, len(points)-1), dtype=int)
        for i in arrow_indices:
            p1, p2 = points[i], points[i+1]
            direction = p2 - p1
            midpoint = (p1 + p2) / 2
            ax.quiver(midpoint[0], midpoint[1], midpoint[2],
                     direction[0], direction[1], direction[2],
                     color='red', length=0.1, normalize=True)
        
        # Mark start and end points
        ax.scatter(*points[0], c='green', s=100, label='Start', marker='*')
        ax.scatter(*points[-1], c='red', s=100, label='End', marker='*')
        
        # Set labels and title
        ax.set_xlabel('X')
        ax.set_ylabel('Y')
        ax.set_zlabel('Z')
        
        view_names = ['Isometric', 'Front', 'Side', 'Top']
        ax.set_title(f'{view_names[idx-1]} View')
        
        # Set viewing angle
        ax.view_init(elev=elev, azim=azim)
        
        # Set equal aspect ratio
        ax.set_box_aspect([1,1,1])
        
        # Add legend
        ax.legend()
        
        # Add colorbar if it's the last subplot
        if idx == len(views):
            plt.colorbar(scatter, ax=ax, label='Position along curve')
    
    plt.tight_layout()
    
    if save_path:
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        plt.close()
    else:
        plt.show()

def interleave3(x, y, z):
    """Interleave the bits of x, y, z to produce a 3D Morton code (Z-order)."""
    def split_by_3bits(n):
        n &= 0x1fffff  # 21 bits
        n = (n | (n << 32)) & 0x1f00000000ffff
        n = (n | (n << 16)) & 0x1f0000ff0000ff
        n = (n | (n << 8))  & 0x100f00f00f00f00f
        n = (n | (n << 4))  & 0x10c30c30c30c30c3
        n = (n | (n << 2))  & 0x1249249249249249
        return n
    return split_by_3bits(x) | (split_by_3bits(y) << 1) | (split_by_3bits(z) << 2)

def morton_curve_points(width, height, depth):
    """Generate all (x, y, z) points in Morton order for a 3D grid."""
    points = []
    for x in range(width):
        for y in range(height):
            for z in range(depth):
                points.append((x, y, z))
    # Sort by Morton code
    points.sort(key=lambda p: interleave3(p[0], p[1], p[2]))
    return np.array(points)

def plot_morton3d(width, height, depth, save_path=None):
    """Create a visualization of the 3D Morton (Z-order) curve with multiple views."""
    points = morton_curve_points(width, height, depth)
    views = [
        (45, 45),   # Isometric view
        (0, 0),     # Front view
        (0, 90),    # Side view
        (90, 0),    # Top view
    ]
    fig = plt.figure(figsize=(20, 5))
    for idx, (elev, azim) in enumerate(views, 1):
        ax = fig.add_subplot(1, 4, idx, projection='3d')
        segments = np.concatenate([points[:-1, None, :], points[1:, None, :]], axis=1)
        t = np.linspace(0, 1, len(points)-1)
        colors_array = plt.cm.plasma(t)
        lc = Line3DCollection(segments, colors=colors_array)
        ax.add_collection3d(lc)
        scatter = ax.scatter(points[:, 0], points[:, 1], points[:, 2],
                           c=np.linspace(0, 1, len(points)),
                           cmap='plasma', s=20)
        # Add arrows at regular intervals to show direction
        arrow_indices = np.linspace(0, len(points)-2, min(20, len(points)-1), dtype=int)
        for i in arrow_indices:
            p1, p2 = points[i], points[i+1]
            direction = p2 - p1
            midpoint = (p1 + p2) / 2
            ax.quiver(midpoint[0], midpoint[1], midpoint[2],
                     direction[0], direction[1], direction[2],
                     color='red', length=0.1, normalize=True)
        ax.scatter(*points[0], c='green', s=100, label='Start', marker='*')
        ax.scatter(*points[-1], c='red', s=100, label='End', marker='*')
        ax.set_xlabel('X')
        ax.set_ylabel('Y')
        ax.set_zlabel('Z')
        view_names = ['Isometric', 'Front', 'Side', 'Top']
        ax.set_title(f'{view_names[idx-1]} View')
        ax.view_init(elev=elev, azim=azim)
        ax.set_box_aspect([1,1,1])
        ax.legend()
        if idx == len(views):
            plt.colorbar(scatter, ax=ax, label='Position along curve')
    plt.tight_layout()
    if save_path:
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        plt.close()
    else:
        plt.show()

def main():
    parser = argparse.ArgumentParser(description='Visualize 3D space-filling curves')
    parser.add_argument('width', type=int, help='Width of the curve')
    parser.add_argument('height', type=int, help='Height of the curve')
    parser.add_argument('depth', type=int, help='Depth of the curve')
    parser.add_argument('--curve', '-c', type=str, choices=['hilbert', 'morton'], default='hilbert', help='Curve type to visualize')
    parser.add_argument('--output', '-o', type=str, help='Output file path')
    args = parser.parse_args()
    if args.curve == 'hilbert':
        plot_hilbert3d_princeton(args.width, args.height, args.depth, args.output)
    else:
        plot_morton3d(args.width, args.height, args.depth, args.output)

if __name__ == "__main__":
    main() 